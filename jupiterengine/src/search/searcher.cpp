#include "searcher.h"
#include "board/boardState.h"
#include "util/executionTimer.h"
#include "util/log.h"
#include "movegen/move.h"
#include "evaluation/evaluator.h"
#include "movegen/moveStream.h"
#include "transpositionTable.h"
#include "board/zobrist.h"
#include "util/instrumenter.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <utility>

Searcher::Searcher(Zobrist& zobrist, PieceSquareTables& pieceSquareTables)
    : m_Zobrist{zobrist}, m_PieceSquareTables{pieceSquareTables} 
{
    JUPITER_TRACE();

    m_FastRNG.Warm();
}

Move Searcher::FindBest(BoardState& state, History& history, uint64_t msRemaining)
{
    JUPITER_TRACE();

    // Keeping these in object scope so all the functions can edit them
    m_SearchAborted = false;
    nodesSearched = 0;
    nodesQuiesced = 0;
    nodesLookedUp = 0;

    // Clear killer moves
    for (std::size_t i = 0; i < MAX_PLY; i++)
        m_Killers[i].Resize(0);

    m_Timer = ExecutionTimer();
    uint64_t startMs = m_Timer.Now();

    // If still in opening look up a book move
    Move openingMove = PickOpeningMove(state);
    if (openingMove.IsValid())
        return openingMove;

    // Decide how long to search for
    CalculateSearchTime(msRemaining);

    Move finalMove = Move::Invalid();
    uint16_t depth = 0;
    while (++depth) {
        // Check if over time every 4096 nodes
        if (((nodesSearched & 4095) == 0 && m_Timer.Now() >= m_SoftSearchBound) || depth >= MAX_PLY)
            break;

        nodesSearched++;

        int16_t depthUnits = depth * PLY_UNIT;

        int32_t alpha = -INFINITY_EVAL;
        int32_t beta = INFINITY_EVAL;
        int32_t bestScore = -INFINITY_EVAL;
        Move bestMove = Move::Invalid();

        Move ttMove = Move::Invalid();
        const TableEntry entry = m_TranspositionTable.Get(state.zobristKey);
        if (entry.IsValid() && entry.depth >= depth) {
            nodesLookedUp++;

            // Only consider exact matches at root (no alpha or beta updates)
            if (entry.nodeType == NodeType::EXACT) {
                finalMove = entry.bestMove;
                continue;
            }

            ttMove = entry.bestMove;
        }

        bool isFirstMove = true;
        Move move;
        Movegen generator(state, m_AttackTable);
        MoveStream movegen = MoveStream(state, generator, &m_Eval, &m_HistoryTable, &m_Killers[0], ttMove);
        while ((move = movegen.Stream()).IsValid()) {
            // Make move and check legality
            MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
            if (!state.WasLegalMove(m_AttackTable, moveData)) {
                state.UnmakeMove(moveData);
                continue;
            }

            // Update state and search
            history.Push(state);
            int32_t score = 0;

            int16_t reduction = 0;
            if (movegen.LastWasBadAttack()) // Reduce moves with negative SEE by one ply
                reduction += PLY_UNIT;
            
            if (!history.IsRepetition()) {
                int16_t nextDepth = depthUnits - PLY_UNIT - reduction;
                if (isFirstMove) {
                    // First (assumed best) move searched with full window
                    score = -Search(state, history, -beta, -alpha, nextDepth, 1);
                } else {
                    // Subsequent moves searched first with null window to check for alpha increase
                    score = -Search(state, history, -alpha - 1, -alpha, nextDepth, 1);
                    if (score > alpha && score < beta) {
                        // If the move raised alpha then re-search with full window
                        score = -Search(state, history, -beta, -alpha, nextDepth, 1);
                    }
                }
            }

            // Undo move
            history.Pop();
            state.UnmakeMove(moveData);
            isFirstMove = false;

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

            if (score >= beta) {
                // History heuristic
                m_HistoryTable[move.from][move.to] += depth * depth;

                // Killer move
                if (movegen.LastWasQuiet() && move != m_Killers[0][0]) {
                    m_Killers[0][1] = m_Killers[0][0];
                    m_Killers[0][0] = move;
                }
                break;
            }
        }

        // Discard search result if aborted (not complete) or not valid (end of game)
        if (m_SearchAborted || !bestMove.IsValid())
            break;

        finalMove = bestMove;
    }

    searchDepth = (m_SearchAborted) ? depth - 1 : depth;
    searchTime = m_SoftSearchBound - startMs;
    ttSize = m_TranspositionTable.OccupancyBytes();

    if (searchDepth <= 3)
        WARN("Extremely low search depth detected");

    return finalMove;
}

int32_t Searcher::Search(BoardState& state, History& history, int32_t alpha, int32_t beta, int16_t depthUnits, uint8_t ply)
{
    JUPITER_TRACE();

    // Check if over time every 4096 nodes
    if ((nodesSearched & 4095) == 0 && m_Timer.Now() >= m_SoftSearchBound) {
        m_SearchAborted = true;
        return 0;
    }

    // Check if finished with this search
    if (depthUnits <= 0)
        return Quiesce(state, history, alpha, beta, ply);

    nodesSearched++;

    int32_t startAlpha = alpha;

    int32_t bestScore = -INFINITY_EVAL;
    Move bestMove = Move::Invalid();

    Move ttMove = Move::Invalid();
    const TableEntry entry = m_TranspositionTable.Get(state.zobristKey);
    if (entry.IsValid() && entry.depth >= depthUnits / PLY_UNIT) {
        nodesLookedUp++;
        ttMove = entry.bestMove;

        int32_t score = entry.score;
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

    bool isFirstMove = true;
    Move move;
    Movegen generator(state, m_AttackTable);
    MoveStream movegen = MoveStream(state, generator, &m_Eval, &m_HistoryTable, &m_Killers[ply], ttMove);
    while ((move = movegen.Stream()).IsValid()) {
        // Make move and check legality
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        if (!state.WasLegalMove(m_AttackTable, moveData)) {
            state.UnmakeMove(moveData);
            continue;
        }

        // Update state and search
        history.Push(state);
        int32_t score = 0;

        int16_t reduction = 0;
        if (movegen.LastWasBadAttack()) // Reduce moves with negative SEE by one ply
            reduction += PLY_UNIT;

        if (!history.IsRepetition()) {
            int16_t nextDepth = depthUnits - PLY_UNIT - reduction;
            if (isFirstMove) {
                // First (assumed best) move searched with full window
                score = -Search(state, history, -beta, -alpha, nextDepth, ply + 1);
            } else {
                // Subsequent moves searched first with null window to check for alpha increase
                score = -Search(state, history, -alpha - 1, -alpha, nextDepth, ply + 1);
                if (score > alpha && score < beta) {
                    // If the move raised alpha then re-search with full window
                    score = -Search(state, history, -beta, -alpha, nextDepth, ply + 1);
                }
            }
        }

        // Undo move
        history.Pop();
        state.UnmakeMove(moveData);
        isFirstMove = false;

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

        if (score >= beta) {
            // History heuristic
            m_HistoryTable[move.from][move.to] += (depthUnits / PLY_UNIT) * (depthUnits / PLY_UNIT);

            // Killer move
            if (movegen.LastWasQuiet() && move != m_Killers[0][0]) {
                m_Killers[0][1] = m_Killers[0][0];
                m_Killers[0][0] = move;
            }
            break;
        }
    }

    // Checkmate or stalemate
    if (!bestMove.IsValid())
        bestScore = IsCheckmate(state) ? -MATE_EVAL + ply : 0;

    // Save search results to TT
    NodeType::Value nodeType = NodeType::EXACT;
    if (bestScore >= beta)
        nodeType = NodeType::LOWER_BOUND;
    else if (bestScore <= startAlpha)
        nodeType = NodeType::UPPER_BOUND;

    // Score mate without ply in the TT if above the mate threshold
    int32_t ttScore = bestScore;
    if (bestScore > MATE_THRESHOLD)
        ttScore += ply;
    else if (bestScore < -MATE_THRESHOLD)
        ttScore -= ply;
    m_TranspositionTable.Save(state, ttScore, depthUnits / PLY_UNIT, bestMove, nodeType);

    return bestScore;
}

int32_t Searcher::Quiesce(BoardState& state, History& history, int32_t alpha, int32_t beta, uint8_t ply)
{
    JUPITER_TRACE();

    // Only check termination condition every 4096 nodes to save expensive clock calls
    if ((nodesSearched & 4095) == 0 && m_Timer.Now() >= m_SoftSearchBound) {
        m_SearchAborted = true;
        return 0;
    }

    nodesQuiesced++;
    nodesSearched++;

    // Standing Pat is only an option when not in check
    bool inCheck = m_AttackTable.SquareUnderAttack(state, state.pieces.OccupancyMask(state.turn, Piece::KING), Color::Opposite(state.turn));
    int32_t bestScore = (inCheck) ? -INFINITY_EVAL : m_Eval.Evaluate(state);

    // Update search terms
    if (bestScore > alpha)
        alpha = bestScore;

    if (alpha >= beta)
        return bestScore;

    bool isFirstMove = true;
    bool hasLegalMove = false;
    Move bestMove = Move::Invalid();
    Move move;
    Movegen generator(state, m_AttackTable);
    MoveStream movegen = MoveStream(state, generator, &m_Eval, &m_HistoryTable, &m_Killers[ply]);
    // Consider quiet moves if in check
    while ((move = movegen.Stream(!inCheck)).IsValid()) {
        // Delta pruning (only non-promotions when not in check)
        if (!inCheck && !Piece::IsValid(move.promote)) {
            const int32_t DELTA_MARGIN = Piece::Evaluate(Piece::KNIGHT); // TODO: Probably want this value to be slightly higher?
            Piece::Value capture = state.pieces.PieceInSquare(Color::Opposite(state.turn), move.to);
            if (!Piece::IsValid(capture)) // en passant
                capture = Piece::PAWN;

            // If the capture doesn't raise alpha then skip the move
            if (bestScore + Piece::Evaluate(capture) + DELTA_MARGIN < alpha)
                continue;
        }

        // Make move and check legality
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        if (!state.WasLegalMove(m_AttackTable, moveData)) {
            state.UnmakeMove(moveData);
            continue;
        }

        hasLegalMove = true;

        // Update state and search
        history.Push(state);
        int32_t score = 0;
        if (!history.IsRepetition()) {
            if (isFirstMove) {
                // First (assumed best) move searched with full window
                score = -Quiesce(state, history, -beta, -alpha, ply + 1);
            } else {
                // Subsequent moves searched first with null window to check for alpha increase
                score = -Quiesce(state, history, -alpha - 1, -alpha, ply + 1);
                if (score > alpha && score < beta) {
                    // If the move raised alpha then re-search with full window
                    score = -Quiesce(state, history, -beta, -alpha, ply + 1);
                }
            }
        }

        // Undo move
        history.Pop();
        state.UnmakeMove(moveData);
        isFirstMove = false;

        // If search terminated mid-move then we discard the search result
        if (m_SearchAborted)
            return 0;

        // Update search terms
        if (score > bestScore) {
            bestMove = move;
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }

        if (score >= beta) {
            // Killer move (possible since we search quiets if in check)
            if (movegen.LastWasQuiet() && move != m_Killers[ply][0]) {
                m_Killers[ply][1] = m_Killers[ply][0];
                m_Killers[ply][0] = move;
            }
            break;
        }
    }

    // Could be in checkmate
    if (inCheck && !hasLegalMove)
        bestScore = -MATE_EVAL + ply;

    return bestScore;
}

void Searcher::SetTimeControl(uint64_t seconds, uint64_t increment)
{
    JUPITER_TRACE();

    m_TimeControlSeconds = seconds;
    m_TimeControlIncrement = increment;
}

bool Searcher::IsCheckmate(const BoardState& state)
{
    JUPITER_TRACE();

    uint64_t kingBit = state.pieces.OccupancyMask(state.turn, Piece::KING);
    return m_AttackTable.SquareUnderAttack(state, kingBit, Color::Opposite(state.turn));
}

void Searcher::SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth, LineBuffer& pv)
{
    JUPITER_TRACE();

    // Reconstruct engine's line
    BoardState reconstructState(state); // This mangles state so make a copy
    pv.Clear();

    // Save the first move
    pv.PushBack(firstMove);
    reconstructState.MakeMove(m_Zobrist, m_PieceSquareTables, firstMove);

    // Iterate over TT entries to find the moves from here
    Movegen generator(state, m_AttackTable);
    for (uint8_t i = 1; i < depth; i++) {
        TableEntry entry = m_TranspositionTable.Get(reconstructState.zobristKey);
        if (!entry.IsValid() || !entry.bestMove.IsValid())
            break;

        // Try to save the next move in line
        MoveData moveData = reconstructState.MakeMove(m_Zobrist, m_PieceSquareTables, entry.bestMove);

        // If it isn't legal then we terminate the PV search
        if (!reconstructState.WasLegalMove(m_AttackTable, moveData)) {
            reconstructState.UnmakeMove(moveData);
            break;
        }

        // Save the move to the line
        pv.PushBack(entry.bestMove);
    }
}

// TODO: Search extensions (time)
void Searcher::CalculateSearchTime(uint64_t msRemaining)
{
    JUPITER_TRACE();

    float incrementMs = m_TimeControlIncrement * 1000.0;
    uint64_t targetMs = msRemaining / 20.0 + incrementMs / 2.0;

    m_SoftSearchBound = m_Timer.StartTime() + targetMs;
    m_HardSearchBound = m_Timer.StartTime() + targetMs * 2;
}

Move Searcher::PickOpeningMove(const BoardState& state)
{
    JUPITER_TRACE();

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

