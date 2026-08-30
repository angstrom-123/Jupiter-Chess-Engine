#include "searcher.h"
#include "boardState.h"
#include "executionTimer.h"
#include "log.h"
#include "move.h"
#include "evaluator.h"
#include "movegen.h"
#include "transpositionTable.h"
#include "zobrist.h"
#include "instrumenter.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <bit>
#include <utility>

Searcher::Searcher(Zobrist& zobrist, OpeningBook& openingBook)
    : m_Zobrist(zobrist), m_OpeningBook{openingBook} 
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_FastRNG.Warm();
}

Move Searcher::FindBest(BoardState& state, History& history, uint64_t msRemaining)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    // Keeping these in object scope so all the functions can edit them
    m_SearchAborted = false;
    nodesSearched = 0;
    nodesQuiesced = 0;
    nodesLookedUp = 0;

    ExecutionTimer timer;

    // If still in opening look up a book move
    Move openingMove = PickOpeningMove(state);
    if (openingMove.IsValid())
        return openingMove;

    // Decide how long to search for
    uint64_t targetTime = timer.StartTime() + CalculateSearchTime(state, msRemaining);

    Move finalMove = Move::Invalid();
    uint8_t depth = 0;
    while (++depth) {
        // Check if over time every 4096 nodes
        if ((nodesSearched & 4095) == 0 && timer.Now() >= targetTime)
            break;

        nodesSearched++;

        Move bestMove = Move::Invalid();
        int64_t bestScore = -INT64_MAX;
        int64_t alpha = -INT64_MAX;
        int64_t beta = INT64_MAX;

        Move move;
        Movegen movegen = Movegen(state, m_AttackTable);
        while ((move = movegen.Stream(m_Eval)).IsValid()) {
            // Make move and check legality
            MoveData moveData = MakeMove(state, move);
            if (!WasLegal(state, moveData)) {
                UnmakeMove(state, moveData);
                continue;
            }

            // Update state and search
            history.Push(state);
            int64_t score = 0;
            if (!history.IsRepetition())
                score = -Search(state, history, timer, -beta, -alpha, depth - 1, 1, targetTime);

            // Undo move
            history.Pop();
            UnmakeMove(state, moveData);

            // If search terminated mid-move then we discard the search result
            if (m_SearchAborted)
                break;

            // Update search terms
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
                if (score > alpha)
                    alpha = score;
            }

            if (score >= beta)
                break;
        }

        // Discard search result if aborted (not complete) or not valid (end of game)
        if (m_SearchAborted || !bestMove.IsValid())
            break;

        finalMove = bestMove;
    }

    searchDepth = (m_SearchAborted) ? depth - 1 : depth;
    ttSize = m_TranspositionTable.OccupancyBytes();

    if (searchDepth <= 3)
        WARN("Extremely low search depth detected");

    return finalMove;
}

int64_t Searcher::Search(BoardState& state, History& history, ExecutionTimer timer, int64_t alpha, int64_t beta, uint8_t depth, uint8_t ply, uint64_t targetMs)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    int64_t startAlpha = alpha;

    // Check if over time every 4096 nodes
    if ((nodesSearched & 4095) == 0 && timer.Now() >= targetMs) {
        m_SearchAborted = true;
        return 0;
    }

    // Check if finished with this search
    if (depth == 0)
        return Quiesce(state, history, timer, alpha, beta, ply, targetMs);

    nodesSearched++;

    // Look up TT entry for this position
    Move ttMove = Move::Invalid();
    const TableEntry entry = m_TranspositionTable.Get(state.zobristKey);
    if (entry.IsValid() && entry.depth >= depth) {
        nodesLookedUp++;
        ttMove = entry.bestMove;

        int64_t score = entry.score;
        if (entry.score > MATE_THRESHOLD)
            score -= ply;
        else if (entry.score < -MATE_THRESHOLD)
            score += ply;

        switch (entry.nodeType) {
            case NodeType::EXACT:
                return score;
            case NodeType::LOWER_BOUND:
                alpha = std::max(alpha, score);
                break;
            case NodeType::UPPER_BOUND:
                beta = std::min(beta, score);
                break;
        }

        if (alpha >= beta)
            return (entry.nodeType == NodeType::EXACT) ? score : alpha;
    }

    int64_t bestScore = -INT64_MAX;
    Move bestMove = Move::Invalid();
    Move move;
    Movegen movegen = Movegen(state, m_AttackTable, ttMove);
    while ((move = movegen.Stream(m_Eval)).IsValid()) {
        // Make move and check legality
        MoveData moveData = MakeMove(state, move);
        if (!WasLegal(state, moveData)) {
            UnmakeMove(state, moveData);
            continue;
        }

        // Update state and search
        history.Push(state);
        int64_t score = 0;
        if (!history.IsRepetition())
            score = -Search(state, history, timer, -beta, -alpha, depth - 1, ply + 1, targetMs);

        // Undo move
        history.Pop();
        UnmakeMove(state, moveData);

        // If search terminated mid-move then we discard the search result
        if (m_SearchAborted)
            return 0;

        // Update search terms
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            if (score > alpha)
                alpha = score;
        }

        if (score >= beta)
            break;
    }

    // Checkmate or stalemate
    if (!bestMove.IsValid())
        return IsCheckmate(state) ? -MATE_EVAL + ply : 0;

    // Save search results to TT
    NodeType::Value nodeType = NodeType::EXACT;
    if (bestScore >= beta)
        nodeType = NodeType::LOWER_BOUND;
    else if (bestScore <= startAlpha)
        nodeType = NodeType::UPPER_BOUND;

    // Score mate without ply in the TT if above the mate threshold
    int64_t ttScore = bestScore;
    if (bestScore > MATE_THRESHOLD)
        ttScore += ply;
    else if (bestScore < -MATE_THRESHOLD)
        ttScore -= ply;
    m_TranspositionTable.Save(state, ttScore, depth, bestMove, nodeType);

    return bestScore;
}

int64_t Searcher::Quiesce(BoardState& state, History& history, ExecutionTimer timer, int64_t alpha, int64_t beta, uint8_t ply, uint64_t targetMs)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    // Only check termination condition every 4096 nodes to save expensive clock calls
    if ((nodesSearched & 4095) == 0 && timer.Now() >= targetMs) {
        m_SearchAborted = true;
        return 0;
    }

    nodesQuiesced++;
    nodesSearched++;

    // Standing Pat
    int64_t bestScore = m_Eval.Evaluate(state);

    // Update search terms
    if (bestScore > alpha)
        alpha = bestScore;

    if (alpha >= beta)
        return bestScore;

    // TODO
    // Transposition Lookup
    Move ttMove = Move::Invalid();
    TableEntry entry = m_TranspositionTable.Get(state.zobristKey);
    if (entry.IsValid()) {
        nodesLookedUp++;
        ttMove = entry.bestMove;

        int64_t score = entry.score;
        if (entry.score > MATE_THRESHOLD)
            score -= ply;
        else if (entry.score < -MATE_THRESHOLD)
            score += ply;

        switch (entry.nodeType) {
            case NodeType::EXACT:
                return score;
            case NodeType::LOWER_BOUND:
                alpha = std::max(alpha, score);
                break;
            case NodeType::UPPER_BOUND:
                beta = std::min(beta, score);
                break;
        }

        // Fail soft here is really important, stops shuffling around when mate is possible
        if (alpha >= beta)
            return score;
    }

    const uint8_t phase = m_Eval.GamePhase(std::forward<const BoardState>(state));

    Move move;
    Movegen movegen = Movegen(state, m_AttackTable, ttMove);
    while ((move = movegen.Stream(m_Eval, true)).IsValid()) {
        // Delta pruning
        if (phase < 80) { // Don't prune in late game
            const int64_t DELTA_MARGIN = 200;
            if (!Piece::IsValid(move.promote)) { // Don't prune promotions
                Piece::Value capture = state.pieces.PieceInSquare(Color::Opposite(state.turn), move.to);
                if (!Piece::IsValid(capture)) // en passant
                    capture = Piece::PAWN;

                // If the capture doesn't raise alpha then skip the move
                if (bestScore + Piece::Evaluate(capture) + DELTA_MARGIN < alpha)
                    continue;
            }
        }

        // Make move and check legality
        MoveData moveData = MakeMove(state, move);
        if (!WasLegal(state, moveData)) {
            UnmakeMove(state, moveData);
            continue;
        }

        // Update state and search
        history.Push(state);
        int64_t score = 0;
        if (!history.IsRepetition())
            score = -Quiesce(state, history, timer, -beta, -alpha, ply + 1, targetMs);

        // Undo move
        history.Pop();
        UnmakeMove(state, moveData);

        // If search terminated mid-move then we discard the search result
        if (m_SearchAborted)
            return 0;

        // Update search terms
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }

        if (score >= beta)
            break;
    }

    return bestScore;
}

void Searcher::SetTimeControl(uint64_t seconds, uint64_t increment)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_TimeControlSeconds = seconds;
    m_TimeControlIncrement = increment;
}

bool Searcher::IsCheckmate(const BoardState& state)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint64_t kingBit = state.pieces.OccupancyMask(state.turn, Piece::KING);
    return SquareUnderAttack(state, kingBit, Color::Opposite(state.turn));
}

MoveData Searcher::MakeMove(BoardState& state, Move move)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    Piece::Value capture = state.pieces.PieceInSquare(move.to).second;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    // For unmaking the move later
    MoveData moveData = {
        .zobristKey = state.zobristKey,
        .move = move,
        .capture = capture,
        .rights = state.rights,
        .turn = state.turn,
        .enPassantIndex = state.enPassantIndex,
        .fiftyMoveCounter = state.fiftyMoveCounter,
        .halfMoves = state.halfMoves
    };

    // Move piece
    state.pieces.Unset(friendly, move.piece, move.from);
    state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, move.piece, move.from);
    if (Piece::IsValid(move.promote)) {
        state.pieces.Set(friendly, move.promote, move.to);
        state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, move.promote, move.to);
    } else {
        state.pieces.Set(friendly, move.piece, move.to);
        state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, move.piece, move.to);
    }

    // Remove capture
    if (Piece::IsValid(capture)) {
        state.pieces.Unset(enemy, capture, move.to);
        state.zobristKey ^= m_Zobrist.ValueForPiece(enemy, capture, move.to);
    }

    // Move rook if castling
    if (move.piece == Piece::KING && Difference(move.from, move.to) == 2) {
        if (move.from > move.to) {
            state.pieces.Unset(friendly, Piece::ROOK, move.from - 4);
            state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, Piece::ROOK, move.from - 4);
            state.pieces.Set(friendly, Piece::ROOK, move.from - 1);
            state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, Piece::ROOK, move.from - 1);
        } else {
            state.pieces.Unset(friendly, Piece::ROOK, move.from + 3);
            state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, Piece::ROOK, move.from + 3);
            state.pieces.Set(friendly, Piece::ROOK, move.from + 1);
            state.zobristKey ^= m_Zobrist.ValueForPiece(friendly, Piece::ROOK, move.from + 1);
        }
    }

    // Remove pawn if en passant
    if (move.piece == Piece::PAWN && move.to == state.enPassantIndex) {
        uint8_t pawnIndex = (friendly == Color::WHITE) ? move.to + 8 : move.to - 8;
        state.pieces.Unset(enemy, Piece::PAWN, pawnIndex);
        state.zobristKey ^= m_Zobrist.ValueForPiece(enemy, Piece::PAWN, pawnIndex);
    }

    // Avoid updating castling rights after both sides lose the right
    if (state.rights > 0) {
        // Remove castling rights if king moved
        if (move.piece == Piece::KING) {
            state.zobristKey ^= m_Zobrist.ValueForRights(state.rights);
            state.rights = 0;
        }

        // Remove castling rights if rook moved from start square
        if (move.piece == Piece::ROOK) {
            if (move.from == (friendly == Color::WHITE ? 63 : 7)) {
                state.rights &= ~CastlingRight::Kingside(friendly);
                state.zobristKey ^= m_Zobrist.ValueForRights(CastlingRight::Kingside(friendly));
            } else if (move.from == (friendly == Color::WHITE ? 56 : 0)) {
                state.rights &= ~CastlingRight::Queenside(friendly);
                state.zobristKey ^= m_Zobrist.ValueForRights(CastlingRight::Queenside(friendly));
            }
        }

        // Remove castling rights if rook captured on start square
        if (capture == Piece::ROOK) {
            if (move.to == (enemy == Color::WHITE ? 63 : 7)) {
                state.rights &= ~CastlingRight::Kingside(enemy);
                state.zobristKey ^= m_Zobrist.ValueForRights(CastlingRight::Kingside(enemy));
            } else if (move.to == (enemy == Color::WHITE ? 56 : 0)) {
                state.rights &= ~CastlingRight::Queenside(enemy);
                state.zobristKey ^= m_Zobrist.ValueForRights(CastlingRight::Queenside(enemy));
            }
        }
    }

    // Update en passant square if double pawn push
    if (state.enPassantIndex != UINT8_MAX) {
        state.zobristKey ^= m_Zobrist.ValueForEnPassant(state.enPassantIndex);
        state.enPassantIndex = UINT8_MAX;
    }
    if (move.piece == Piece::PAWN && Difference(move.from, move.to) == 16) {
        // Check if any of our pawns can en passant the opponent pawn that just double pushed
        // This means that two identical positions (except for the en passant square) will hash to 
        // the same value as long as there is no pawn to capture en passant.
        // This check only accounts for pseudo-legal en passant captures but is better than nothing.
        uint8_t enPassantIndex = (friendly == Color::WHITE) ? move.to + 8 : move.to - 8;
        uint8_t file = enPassantIndex & 7;
        uint64_t adjacentMask = 0;
        if (file > 0) adjacentMask |= (1ul << (move.to - 1));
        if (file < 7) adjacentMask |= (1ul << (move.to + 1));
        if (adjacentMask & state.pieces.OccupancyMask(enemy, Piece::PAWN)) {
            state.zobristKey ^= m_Zobrist.ValueForEnPassant(enPassantIndex);
            state.enPassantIndex = enPassantIndex;
        }
    }

    // Update 50 move counter
    if (Piece::IsValid(capture) || move.piece == Piece::PAWN)
        state.fiftyMoveCounter = 0;
    else
        state.fiftyMoveCounter++;

    state.turn = enemy;
    state.zobristKey ^= m_Zobrist.ValueForTurn(friendly);
    state.zobristKey ^= m_Zobrist.ValueForTurn(enemy);

    state.halfMoves++;

    return moveData;
}

void Searcher::SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth)
{
    JUPITER_TRACE();

    // TODO: Call this function somewhere

    // Reconstruct engine's line
    BoardState reconstructState(state); // This mangles state so make a copy
    LineBuffer principalVariation;
    principalVariation.Clear();

    // Save the first move
    principalVariation.PushBack(firstMove);
    MakeMove(reconstructState, firstMove);

    // Iterate over TT entries to find the moves from here
    for (uint8_t i = 1; i < depth; i++) {
        TableEntry entry = m_TranspositionTable.Get(reconstructState.zobristKey);
        if (!entry.IsValid() || !entry.bestMove.IsValid())
            break;

        // Try to save the next move in line
        MoveData moveData = MakeMove(reconstructState, entry.bestMove);

        // If it isn't legal then we terminate the PV search
        if (!WasLegal(std::forward<const BoardState>(reconstructState), moveData)) {
            UnmakeMove(reconstructState, moveData);
            break;
        }

        // Save the move to the line
        principalVariation.PushBack(entry.bestMove);
    }
}

// TODO: I think checkmate ply calculations are backwards??
//       Black was just not going for it?
//       Check it for white too 
//       Set up a test position temporarily

// TODO: Search extensions
uint64_t Searcher::CalculateSearchTime(const BoardState& state, uint64_t msRemaining)
{
    JUPITER_TRACE();

    float incrementMs = m_TimeControlIncrement * 1000.0;
    searchTime = msRemaining / 20.0 + incrementMs / 2.0;

    // TODO: Estimate position complexity (just using game phase for now)
    float c = 1.0;
    int64_t phase = m_Eval.GamePhase(std::forward<const BoardState>(state));
    if (phase > 15 && phase < 60)
        c += 0.10;

    searchTime *= c;
    return searchTime;
}

Move Searcher::PickOpeningMove(const BoardState& state)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    if (!m_InOpeningBook)
        return Move::Invalid();

    OpeningMoves moves;
    if (!m_OpeningBook.LookupMoves(std::forward<const BoardState>(state), moves)) {
        m_InOpeningBook = false;
        return Move::Invalid();
    }

    bookMoves++;
    return moves[m_FastRNG.Generate() % moves.Size()].first;
}

void Searcher::UnmakeMove(BoardState& state, MoveData moveData)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    const Move& move = moveData.move;

    Color::Value friendly = moveData.turn;
    Color::Value enemy = Color::Opposite(moveData.turn);

    // Replace moving piece
    state.pieces.Set(friendly, move.piece, move.from);
    if (Piece::IsValid(move.promote))
        state.pieces.Unset(friendly, move.promote, move.to);
    else
        state.pieces.Unset(friendly, move.piece, move.to);

    // Replace capture
    if (Piece::IsValid(moveData.capture))
        state.pieces.Set(enemy, moveData.capture, move.to);

    // Replace rook if castling
    if (move.piece == Piece::KING && Difference(move.from, move.to) == 2) {
        if (move.from > move.to) {
            state.pieces.Set(friendly, Piece::ROOK, move.from - 4);
            state.pieces.Unset(friendly, Piece::ROOK, move.from - 1);
        } else {
            state.pieces.Set(friendly, Piece::ROOK, move.from + 3);
            state.pieces.Unset(friendly, Piece::ROOK, move.from + 1);
        }
    }

    // Replace pawn if en passant
    if (move.piece == Piece::PAWN && move.to == moveData.enPassantIndex)
        state.pieces.Set(enemy, Piece::PAWN, (friendly == Color::WHITE) ? move.to + 8 : move.to - 8);

    // Update variables
    state.rights = moveData.rights;
    state.turn = moveData.turn;
    state.enPassantIndex = moveData.enPassantIndex;
    state.halfMoves = moveData.halfMoves;
    state.fiftyMoveCounter = moveData.fiftyMoveCounter;
    state.zobristKey = moveData.zobristKey;
}

bool Searcher::SquareUnderAttack(const BoardState& state, uint64_t bit, Color::Value color)
{
    JUPITER_TRACE();

    uint8_t index = std::countr_zero(bit);
    Color::Value enemy = Color::Opposite(color);
    Bitboard occupancy = state.pieces.OccupancyMask();

    // Queens
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::QUEEN, enemy, occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::QUEEN))
            return true;
    }

    // Bishops
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::BISHOP, enemy, occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::BISHOP))
            return true;
    }

    // Rooks
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::ROOK, enemy, occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::ROOK))
            return true;
    }

    // Pawns
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::PAWN, enemy, occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::PAWN))
            return true;
    }

    // Knights
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, enemy, occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::KNIGHT))
            return true;
    }

    // Kings
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, enemy, occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::KING))
            return true;
    }

    return false;
}

bool Searcher::WasLegal(const BoardState& state, MoveData moveData)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    Bitboard king = state.pieces.OccupancyMask(moveData.turn, Piece::KING);

    bool targetAttacked = SquareUnderAttack(std::forward<const BoardState>(state), king, Color::Opposite(moveData.turn));

    // Check intermediate and start squares if castling
    if (moveData.move.piece == Piece::KING && Difference(moveData.move.from, moveData.move.to) == 2) {
        bool startAttacked = SquareUnderAttack(std::forward<const BoardState>(state), 1ul << moveData.move.from, Color::Opposite(moveData.turn));
        uint8_t midIndex = (moveData.move.from > moveData.move.to) ? moveData.move.from - 1 : moveData.move.from + 1;
        bool midAttacked = SquareUnderAttack(std::forward<const BoardState>(state), 1ul << midIndex, Color::Opposite(moveData.turn));

        return !(targetAttacked || startAttacked || midAttacked);
    }

    return !targetAttacked;
}
