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
#include <cstring>
#include <ctime>
#include <utility>

Searcher::Searcher(Zobrist& zobrist, PieceSquareTables& pieceSquareTables)
    : m_Zobrist{zobrist}, m_PieceSquareTables{pieceSquareTables} 
{
    JUPITER_TRACE();

    m_FastRNG.Warm();
}

void Searcher::Reset()
{
    // Keeping these in object scope so all the functions can edit them
    m_SearchAborted = false;
    m_LastNodesSearched = 0;
    m_LastNodesQuiesced = 0;
    m_LastNodesLookedUp = 0;

    // Clear killer moves
    for (std::size_t i = 0; i < MAX_PLY; i++)
        m_Killers[i].Resize(0);

    // Reset the timer
    m_Timer = ExecutionTimer();
}

Move Searcher::FindBest(BoardState& state, History& history, uint64_t remainingMs)
{
    JUPITER_TRACE();

    Reset();
    CalculateSearchTime(remainingMs);

    Move finalMove = Move::Invalid();
    if ((finalMove = PickOpeningMove(state)).IsValid())
        return finalMove;

    bool onlyOneMove = false;
    uint16_t depth = 0;
    while (++depth) {
        // Soft search bound checked at each iterative deepening iteration
        if (m_Timer.Now() >= m_SoftSearchBound || depth >= MAX_PLY)
            break;

        m_LastNodesSearched++;

        int16_t depthUnits = depth * PLY_UNIT;
        AlphaBeta window{};
        int32_t bestScore = -INFINITY_EVAL;
        Move bestMove = Move::Invalid();

        // Do not update alpha and beta at the root
        Move ttMove = Move::Invalid();
        const TTEntry entry = m_TranspositionTable.Get(state.zobristKey);
        if (entry.IsValid() && entry.depth >= depth) {
            m_LastNodesLookedUp++;
            if (entry.nodeType == NodeType::EXACT) {
                finalMove = entry.bestMove;
                continue;
            }
            ttMove = entry.bestMove;
        }

        Move move;
        MoveStream movegen = MoveStream(state, Movegen(state, m_AttackTable), &m_Eval, &m_HistoryTable, &m_Killers[0], ttMove);
        while ((move = movegen.Stream()).IsValid()) {
            int32_t score = 0;

            // Search move (if legal)
            {
                MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);

                if (!state.WasLegalMove(m_AttackTable, moveData)) {
                    state.UnmakeMove(moveData);
                    continue;
                }

                history.Push(state);

                if (!history.IsRepetition()) {
                    int16_t nextDepth = depthUnits - PLY_UNIT + CalculateExtension(state, movegen);
                    score = PrincipalVariationSearch(state, history, movegen, window, nextDepth, 1);
                }

                history.Pop();
                state.UnmakeMove(moveData);
            }

            // If search terminated mid-depth then we discard the search result
            if (m_SearchAborted)
                break;

            // Update search terms
            if (score > bestScore) {
                bestScore = score;
                bestMove = move;
                window.alpha = std::max(window.alpha, score);
            }

            if (score >= window.beta) {
                // History heuristic
                m_HistoryTable[move.from][move.to] += depth * depth;

                // Killer move
                if (movegen.IsQuietMove() && move != m_Killers[0][0]) {
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

        // Do not continue to higher depths if there is only one legal move
        if (movegen.GetMoveCount() == 1) {
            onlyOneMove = true;
            break;
        }
    }

    m_LastSearchDepth = (m_SearchAborted) ? depth - 1 : depth;
    m_LastSearchTime = m_Timer.SinceStart<chrono::milliseconds>().count();
    m_LastPawnsLookedUp = m_Eval.GetPawnTableHitCount() - m_LastPawnsLookedUp;
    m_LastEvaluations = m_Eval.GetEvaluationCount() - m_LastEvaluations;

    if (m_LastSearchDepth <= 3 && !onlyOneMove)
        WARN("Extremely low search depth detected");

    return finalMove;
}

int32_t Searcher::Search(BoardState& state, History& history, AlphaBeta window, int16_t depthUnits, uint8_t ply)
{
    JUPITER_TRACE();

    // Check if over time every 4096 nodes
    if ((m_LastNodesSearched & 4095) == 0 && m_Timer.Now() >= m_HardSearchBound) {
        m_SearchAborted = true;
        return 0;
    }

    // Check if finished with this search
    if (depthUnits <= 0)
        return Quiesce(state, history, window, ply);

    m_LastNodesSearched++;

    int32_t startAlpha = window.alpha;

    int32_t bestScore = -INFINITY_EVAL;
    Move bestMove = Move::Invalid();

    Move ttMove = Move::Invalid();
    const TTEntry entry = m_TranspositionTable.Get(state.zobristKey);
    if (entry.IsValid() && entry.depth >= depthUnits / PLY_UNIT) {
        m_LastNodesLookedUp++;
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
                window.alpha = std::max(window.alpha, score);
                break;
            case NodeType::UPPER_BOUND:
                window.beta = std::min(window.beta, score);
                break;
        }

        if (window.IsBetaCutoff())
            return (entry.nodeType == NodeType::EXACT) ? score : window.alpha;
    }

    Move move;
    Movegen generator(state, m_AttackTable);
    MoveStream movegen = MoveStream(state, generator, &m_Eval, &m_HistoryTable, &m_Killers[ply], ttMove);
    while ((move = movegen.Stream()).IsValid()) {
        int32_t score = 0;

        // Search move (if legal)
        {
            MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);

            if (!state.WasLegalMove(m_AttackTable, moveData)) {
                state.UnmakeMove(moveData);
                continue;
            }

            history.Push(state);

            if (!history.IsRepetition()) {
                int16_t nextDepth = depthUnits - PLY_UNIT + CalculateExtension(state, movegen);
                score = PrincipalVariationSearch(state, history, movegen, window, nextDepth, ply + 1);
            }

            history.Pop();
            state.UnmakeMove(moveData);
        }

        if (m_SearchAborted)
            return 0; // The result is discarded at the call site

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            window.alpha = std::max(window.alpha, score);
        }

        if (score >= window.beta) {
            // History heuristic
            uint8_t depth = depthUnits / PLY_UNIT;
            m_HistoryTable[move.from][move.to] += depth * depth;

            // Killer move
            if (movegen.IsQuietMove() && move != m_Killers[0][0]) {
                m_Killers[0][1] = m_Killers[0][0];
                m_Killers[0][0] = move;
            }
            break;
        }
    }

    // Checkmate or stalemate
    if (!bestMove.IsValid())
        bestScore = KingUnderAttack(state, state.turn) ? -MATE_EVAL + ply : 0;

    // Save search results to TT
    NodeType::Value nodeType = NodeType::EXACT;
    if (bestScore >= window.beta)
        nodeType = NodeType::LOWER_BOUND;
    else if (bestScore <= startAlpha)
        nodeType = NodeType::UPPER_BOUND;

    // Score mate without ply in the TT if above the mate threshold
    int32_t ttScore = bestScore;
    if (bestScore > MATE_THRESHOLD)
        ttScore += ply;
    else if (bestScore < -MATE_THRESHOLD)
        ttScore -= ply;
    m_TranspositionTable.Save(state.zobristKey, state.halfMove, ttScore, depthUnits / PLY_UNIT, bestMove, nodeType);

    return bestScore;
}

int32_t Searcher::Quiesce(BoardState& state, History& history, AlphaBeta window, uint8_t ply)
{
    JUPITER_TRACE();

    // Only check termination condition every 4096 nodes to save expensive clock calls
    if ((m_LastNodesSearched & 4095) == 0 && m_Timer.Now() >= m_HardSearchBound) {
        m_SearchAborted = true;
        return 0;
    }

    m_LastNodesQuiesced++;
    m_LastNodesSearched++;

    // Standing Pat (lower bound on score) is only an option when not in check
    bool inCheck = KingUnderAttack(state, state.turn);
    int32_t bestScore = (inCheck) ? -INFINITY_EVAL : m_Eval.Evaluate(state);

    window.alpha = std::max(window.alpha, bestScore);
    if (window.IsBetaCutoff())
        return bestScore;

    Move bestMove = Move::Invalid();
    Move move;
    Movegen generator(state, m_AttackTable);
    MoveStream movegen = MoveStream(state, generator, &m_Eval, &m_HistoryTable, &m_Killers[ply]);
    // Consider quiet moves if in check
    while ((move = movegen.Stream(!inCheck)).IsValid()) {
        // Delta pruning (only non-promotions when not in check)
        if (!inCheck && !Piece::IsValid(move.promote)) {
            const int32_t DELTA_MARGIN = Piece::Evaluate(Piece::KNIGHT); // TODO: This should be tunable?
            Piece::Value capture = state.pieces.PieceInSquare(Color::Opposite(state.turn), move.to);
            if (!Piece::IsValid(capture)) // en passant
                capture = Piece::PAWN;

            // If the capture doesn't raise alpha then skip the move
            if (bestScore + Piece::Evaluate(capture) + DELTA_MARGIN < window.alpha)
                continue;
        }

        // Make move and check legality
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        if (!state.WasLegalMove(m_AttackTable, moveData)) {
            state.UnmakeMove(moveData);
            continue;
        }

        // Update state and search
        history.Push(state);

        int32_t score = 0;
        if (!history.IsRepetition())
            score = PrincipalVariationQuiesce(state, history, movegen, window, ply + 1);

        // Undo move
        history.Pop();
        state.UnmakeMove(moveData);

        // If search terminated mid-move then we discard the search result
        if (m_SearchAborted)
            return 0;

        // Update search terms
        if (score > bestScore) {
            bestMove = move;
            bestScore = score;
            window.alpha = std::max(window.alpha, score);
        }

        if (score >= window.beta) {
            // Killer move (possible since we search quiets if in check)
            if (movegen.IsQuietMove() && move != m_Killers[ply][0]) {
                m_Killers[ply][1] = m_Killers[ply][0];
                m_Killers[ply][0] = move;
            }
            break;
        }
    }

    // Could be in checkmate
    if (inCheck && !bestMove.IsValid())
        bestScore = -MATE_EVAL + ply;

    return bestScore;
}

int32_t Searcher::PrincipalVariationSearch(BoardState& state, History& history, const MoveStream& movegen, AlphaBeta window, int16_t depthUnits, uint8_t nextPly)
{
    if (movegen.IsFirstMove()) {
        // First (assumed best) move searched with full window
        return -Search(state, history, -window, depthUnits, nextPly);
    } else {
        // Subsequent moves searched first with null window to check for alpha increase
        int32_t score = -Search(state, history, window.Null(), depthUnits, nextPly);
        if (window.Contains(score)) {
            // If the move raised alpha then re-search with full window
            score = -Search(state, history, -window, depthUnits, nextPly);
        }
        return score;
    }
}

int32_t Searcher::PrincipalVariationQuiesce(BoardState& state, History& history, const MoveStream& movegen, AlphaBeta window, uint8_t nextPly)
{
    if (movegen.IsFirstMove()) {
        // First (assumed best) move searched with full window
        return -Quiesce(state, history, -window, nextPly);
    } else {
        // Subsequent moves searched first with null window to check for alpha increase
        int32_t score = -Quiesce(state, history, window.Null(), nextPly);
        if (window.Contains(score)) {
            // If the move raised alpha then re-search with full window
            score = -Quiesce(state, history, -window, nextPly);
        }
        return score;
    }
}

void Searcher::SetTimeControl(float seconds, float increment)
{
    JUPITER_TRACE();

    m_TimeControlSeconds = seconds;
    m_TimeControlIncrement = increment;
}

void Searcher::TelemetryJSON(std::string& result) const 
{
    JUPITER_TRACE();

    std::ostringstream ss;

    ss << "{"
        << "\"depth\":" << (int) m_LastSearchDepth << ","
        << "\"nodesSearched\":" << m_LastNodesSearched << ","
        << "\"nodesLookedUp\":" << m_LastNodesLookedUp << ","
        << "\"nodesQuiesced\":" << m_LastNodesQuiesced << ","
        << "\"pawnsLookedUp\":" << m_LastPawnsLookedUp << ","
        << "\"evaluations\":" << m_LastEvaluations << ","
        << "\"searchTime\":" << m_LastSearchTime 
        << "}";

    result = ss.str();
}

void Searcher::MetricsJSON(std::string& result) const 
{
    JUPITER_TRACE();

    std::ostringstream ss;

    ss << "{"
        << "\"ttSize\":" << m_TranspositionTable.OccupancyBytes() << ","
        << "\"ptSize\":" << m_Eval.GetPawnTableSize() << ","
        << "\"bookMoves\":" << (int) m_BookMoves
        << "}";

    result = ss.str();
}

void Searcher::TuneEval(const EvaluatorConstants& weights)
{
    JUPITER_TRACE();

    m_Eval.Tune(std::forward<const EvaluatorConstants&>(weights));
}

EvaluatorConstants Searcher::EvalWeights() const
{
    JUPITER_TRACE();

    return m_Eval.GetWeights();
}

bool Searcher::KingUnderAttack(const BoardState& state, Color::Value turn)
{
    JUPITER_TRACE();

    uint64_t kingBit = state.pieces.Occupancy(turn, Piece::KING);
    return m_AttackTable.SquareUnderAttack(state, kingBit, Color::Opposite(turn));
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
        TTEntry entry = m_TranspositionTable.Get(reconstructState.zobristKey);
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

int16_t Searcher::CalculateExtension(const BoardState& state, const MoveStream& movegen)
{
    int16_t extension = 0;

    // Attacks with negative SEE searched shallower
    if (movegen.IsBadAttack())
        extension -= PLY_UNIT;

    // Extend when giving or receiving check
    if (KingUnderAttack(state, state.turn) || KingUnderAttack(state, Color::Opposite(state.turn)))
        extension += PLY_UNIT;

    return extension;
}

// TODO: Search extensions (time)
void Searcher::CalculateSearchTime(uint64_t remainingMs)
{
    JUPITER_TRACE();

    float incrementMs = m_TimeControlIncrement * 1000.0;
    uint64_t targetMs = remainingMs / 20.0 + incrementMs / 2.0;

    m_SoftSearchBound = m_Timer.StartTime() + chrono::milliseconds(targetMs);
    m_HardSearchBound = m_Timer.StartTime() + chrono::milliseconds(static_cast<uint64_t>(targetMs * 1.5)); // NOTE: Reduced this to stop timing out
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

    m_BookMoves++;
    return moves[m_FastRNG.Generate() % moves.Size()].first;
}

