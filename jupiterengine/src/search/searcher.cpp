#include "searcher.h"
#include "board/boardState.h"
#include "movegen/movegen.h"
#include "util/executionTimer.h"
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
    JUPITER_TRACE();

    m_SearchAborted = false;
    m_LastNodesSearched = 0;
    m_LastNodesQuiesced = 0;
    m_LastNodesLookedUp = 0;
    m_Timer = ExecutionTimer();

    for (std::size_t i = 0; i < MAX_KILLERS + 1; i++) // + 1 here for the null slot at the end
        m_Killers[i].Clear();
}

Move Searcher::FindBest(BoardState& state, History& history, uint64_t remainingMs)
{
    JUPITER_TRACE();

    Reset();
    CalculateSearchTime(remainingMs);

    Move finalMove = Move::Invalid();
    if ((finalMove = PickOpeningMove(state)).IsValid())
        return finalMove;

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

        // Initialise movegen
        MoveStream movegen(state, m_AttackTable, m_Eval, m_HistoryTable, m_Killers[0], false);

        // Do not update alpha and beta at the root
        TranspositionTableEntry entry = m_TranspositionTable.Probe(state.zobristKey);
        if (entry.IsValid() && entry.depth >= depth && state.IsPseudoLegal(movegen.GetAttacks(), movegen.GetQuiets(), entry.bestMove)) {
            // Check legality of cache move
            MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, entry.bestMove);
            if (state.WasLegalMove(m_AttackTable, moveData)) {
                m_LastNodesLookedUp++;
                if (entry.cutoff == Cutoff::NONE) {
                    finalMove = entry.bestMove;
                    state.UnmakeMove(moveData); // Have to undo in here too because of continue
                    continue;
                }
                movegen.SetTTMove(entry.bestMove);
            }
            state.UnmakeMove(moveData);
        }

        Move move;
        while ((move = movegen.Stream()).IsValid()) {
            int32_t score = 0;

            // Search move (if legal)
            MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
            if (!state.WasLegalMove(m_AttackTable, moveData)) {
                state.UnmakeMove(moveData);
                continue;
            }
            // Prefetch transposition table entry into the caches
            Prefetch(m_TranspositionTable.Line(state.zobristKey));

            history.Push(state);

            if (!history.IsRepetition()) {
                int16_t nextDepth = depthUnits - PLY_UNIT + CalculateExtension(state, movegen);
                score = PrincipalVariationSearch(state, history, movegen, window, nextDepth, 1);
            }

            history.Pop();
            state.UnmakeMove(moveData);

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
                if (movegen.IsQuietMove() && !m_Killers[0].Contains(move))
                    m_Killers[0].Push(move);
                break;
            }
        }

        // Discard search result if aborted (not complete) or not valid (end of game)
        if (m_SearchAborted || !bestMove.IsValid())
            break;

        finalMove = bestMove;

        // Do not continue to higher depths if there is only one legal move
        if (movegen.GetMoveCount() == 1)
            break;
    }

    // Save some info about the last search
    m_LastSearchDepth = (m_SearchAborted) ? depth - 1 : depth;
    m_LastSearchTime = m_Timer.SinceStart<chrono::milliseconds>().count();
    m_LastPawnsLookedUp = m_Eval.GetPawnTableHitCount() - m_LastPawnsLookedUp;
    m_LastEvaluations = m_Eval.GetEvaluationCount() - m_LastEvaluations;

    return finalMove;
}

uint64_t Searcher::Perft(BoardState& state, uint8_t depth)
{
	JUPITER_TRACE();

	if (depth == 0)
		return 1;

	return PerftRecurse(state, depth);
}

uint64_t Searcher::PerftDivide(BoardState& state, uint8_t depth, PerftBuffer& buffer)
{
	JUPITER_TRACE();

	if (depth == 0)
		return 1;

	uint64_t total = 0;

    // Count all attacks
	AttackMoveBuffer attacks{};
    movegen::FindAllAttacks(state, m_AttackTable, attacks);
	for (const Move move : attacks) {
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
		if (state.WasLegalMove(m_AttackTable, moveData)) {
			uint64_t n = PerftRecurse(state, depth - 1);
			total += n;

            // Save the count for this starting move to the buffer
			buffer.PushBack(std::make_pair(move, n));
		}
		state.UnmakeMove(moveData);
	}

    // Count all quiets
	QuietMoveBuffer quiets{};
    movegen::FindAllQuiets(state, m_AttackTable, quiets);
	for (const Move move : quiets) {
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
		if (state.WasLegalMove(m_AttackTable, moveData)) {
			uint64_t n = PerftRecurse(state, depth - 1);
			total += n;

            // Save the count for this starting move to the buffer
			buffer.PushBack(std::make_pair(move, n));
		}
		state.UnmakeMove(moveData);
	}

	return total;
}


uint64_t Searcher::PerftRecurse(BoardState& state, uint8_t depth)
{
	JUPITER_TRACE();

	if (depth == 0)
		return 1;

	uint64_t count = 0;

	AttackMoveBuffer attacks{};
    movegen::FindAllAttacks(state, m_AttackTable, attacks);
	for (const Move move : attacks) {
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
		if (state.WasLegalMove(m_AttackTable, moveData))
			count += PerftRecurse(state, depth - 1);
		state.UnmakeMove(moveData);
	}

	QuietMoveBuffer quiets{};
    movegen::FindAllQuiets(state, m_AttackTable, quiets);
	for (const Move move : quiets) {
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
		if (state.WasLegalMove(m_AttackTable, moveData))
			count += PerftRecurse(state, depth - 1);
		state.UnmakeMove(moveData);
	}

	return count;
}

int32_t Searcher::Search(BoardState& state, History& history, AlphaBeta window, int16_t depthUnits, uint8_t ply)
{
    JUPITER_TRACE();

    // Check if over time every 4096 nodes
    if ((m_LastNodesSearched & 4095) == 0 && m_Timer.Now() >= m_HardSearchBound) {
        m_SearchAborted = true;
        return 0;
    }

    // Start quiescence if finished with this search
    if (depthUnits <= 0)
        return Quiesce(state, history, window, ply);

    m_LastNodesSearched++;

    int32_t startAlpha = window.alpha;

    int32_t bestScore = -INFINITY_EVAL;
    Move bestMove = Move::Invalid();

    // Initialise movegen
    MoveStream movegen(state, m_AttackTable, m_Eval, m_HistoryTable, m_Killers[std::min(ply, MAX_PLY)], false);

    TranspositionTableEntry entry = m_TranspositionTable.Probe(state.zobristKey);
    if (entry.IsValid() && entry.depth >= depthUnits / PLY_UNIT && state.IsPseudoLegal(movegen.GetAttacks(), movegen.GetQuiets(), entry.bestMove)) {
        // Check legality of cache move
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, entry.bestMove);
        if (state.WasLegalMove(m_AttackTable, moveData)) {
            m_LastNodesLookedUp++;
            movegen.SetTTMove(entry.bestMove);
            int32_t score = TranspositionTable::DenormalizeScore(entry.score, ply);

            if (entry.cutoff == Cutoff::NONE) {
                state.UnmakeMove(moveData); // Unmake here because of return
                return score;
            }

            window.Update(entry.cutoff, score);
            if (window.IsBetaCutoff()) { // Beta cutoff means opponent won't let this branch happen
                state.UnmakeMove(moveData); // Unmake here because of return
                return window.alpha; 
            }
        }
        state.UnmakeMove(moveData);
    }

    Move move;
    while ((move = movegen.Stream()).IsValid()) {
        int32_t score = 0;

        // Search move (if legal)
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        if (!state.WasLegalMove(m_AttackTable, moveData)) {
            state.UnmakeMove(moveData);
            continue;
        }

        // Prefetch transposition table entry into the caches
        Prefetch(m_TranspositionTable.Line(state.zobristKey));

        history.Push(state);

        if (!history.IsRepetition()) {
            int16_t nextDepth = depthUnits - PLY_UNIT + CalculateExtension(state, movegen);
            score = PrincipalVariationSearch(state, history, movegen, window, nextDepth, ply + 1);
        }

        history.Pop();
        state.UnmakeMove(moveData);

        // Return a dummy value, discarded at call site
        if (m_SearchAborted)
            return 0;

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            window.alpha = std::max(window.alpha, score);
        }

        if (score >= window.beta) {
            // History heuristic
            uint8_t depth = depthUnits / PLY_UNIT;
            m_HistoryTable[move.from][move.to] += depth * depth;

            // Killer moves
            if (ply < MAX_PLY) {
                if (movegen.IsQuietMove() && !m_Killers[ply].Contains(move)) {
                    m_Killers[ply].Push(move);
                }
            }
            break;
        }
    }

    // Checkmate or stalemate + early exit, DO NOT STORE INVALIDS TO TT
    if (!bestMove.IsValid())
        return state.KingUnderAttack(m_AttackTable) ? -MATE_EVAL + ply : 0;

    // Save to transposition table
    m_TranspositionTable.Store(TranspositionTableEntry {
        .hash = state.zobristKey, 
        .score = TranspositionTable::NormalizeScore(bestScore, ply),
        .bestMove = bestMove,
        .halfMove = state.halfMove,
        .depth = static_cast<uint8_t>(depthUnits / PLY_UNIT),
        .cutoff = TranspositionTable::ClassifyCutoff(bestScore, startAlpha, window.beta)
    });

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
    bool inCheck = state.KingUnderAttack(m_AttackTable);
    int32_t bestScore = (inCheck) ? -INFINITY_EVAL : m_Eval.Evaluate(state);

    window.alpha = std::max(window.alpha, bestScore);
    if (window.IsBetaCutoff())
        return bestScore;

    Move bestMove = Move::Invalid();

    // Quiescence mode, quiet moves only considered when in check
    MoveStream movegen(state, m_AttackTable, m_Eval, m_HistoryTable, m_Killers[std::min(ply, MAX_PLY)], !inCheck);
    Move move;
    while ((move = movegen.Stream()).IsValid()) {
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

        int32_t score = 0;

        // Search move (if legal)
        MoveData moveData = state.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        if (!state.WasLegalMove(m_AttackTable, moveData)) {
            state.UnmakeMove(moveData);
            continue;
        }

        history.Push(state);

        if (!history.IsRepetition())
            score = PrincipalVariationQuiesce(state, history, movegen, window, ply + 1);

        history.Pop();
        state.UnmakeMove(moveData);

        // Return a dummy value, discarded at call site
        if (m_SearchAborted)
            return 0;

        // Update search terms
        if (score > bestScore) {
            bestMove = move;
            bestScore = score;
            window.alpha = std::max(window.alpha, score);
        }

        if (score >= window.beta) {
            // Killer moves are strictly quiet moves which are avoided in quiescence. We can see 
            // quiets here if a king is in check and must evade it
            if (ply < MAX_PLY) {
                if (movegen.IsQuietMove() && !m_Killers[ply].Contains(move)) {
                    m_Killers[ply].Push(move);
                }
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
    JUPITER_TRACE();

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
    JUPITER_TRACE();

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
    for (uint8_t i = 1; i < depth; i++) {
        TranspositionTableEntry entry = m_TranspositionTable.Probe(reconstructState.zobristKey);
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
    JUPITER_TRACE();

    int16_t extension = 0;

    // Attacks with negative SEE searched shallower
    if (movegen.IsBadAttack())
        extension -= PLY_UNIT;

    // Extend when giving or receiving check
    if (state.KingUnderAttack(m_AttackTable) || state.KingUnderAttack(m_AttackTable, Color::Opposite(state.turn)))
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

