#pragma once

#include "board/boardState.h"
#include "board/history.h"
#include "datastructure/buffer.h"
#include "evaluation/evaluator.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"
#include "movegen/attackTable.h"
#include "movegen/moveStream.h"
#include "search/window.h"
#include "util/executionTimer.h"
#include "util/rng.h"
#include "search/openingBook.h"
#include "search/transpositionTable.h"

using LineBuffer = Buffer<Move, MAX_PLY>;
using PerftBuffer = Buffer<std::pair<Move, uint64_t>, MAX_ATTACKS + MAX_QUIETS>;

constexpr uint16_t PLY_UNIT = 64;
constexpr uint16_t HALF_PLY_UNIT = PLY_UNIT / 2;

class Searcher {
public:
    Searcher(Zobrist& zobrist, PieceSquareTables& pieceSquareTables);
    Move FindBest(BoardState& state, History& history, uint64_t remainingMs);
	uint64_t Perft(BoardState& state, uint8_t depth);
	uint64_t PerftDivide(BoardState& state, uint8_t depth, PerftBuffer& buffer);
    void SetTimeControl(float seconds, float increment);
    void TelemetryJSON(std::string& result) const;
    void MetricsJSON(std::string& result) const;
    void TuneEval(const EvaluatorConstants& weights);
    EvaluatorConstants EvalWeights() const;

private:
	uint64_t PerftRecurse(BoardState& state, uint8_t depth);
    void SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth, LineBuffer& pv);
    void CalculateSearchTime(uint64_t remainingMs);
    int16_t CalculateExtension(const BoardState& state, const MoveStream& movegen);
    Move PickOpeningMove(const BoardState& state);
    int32_t Search(BoardState& state, History& history, AlphaBeta window, int16_t depthUnits, uint8_t ply);
    int32_t Quiesce(BoardState& state, History& history, AlphaBeta window, uint8_t ply);
    int32_t PrincipalVariationSearch(BoardState& state, History& history, const MoveStream& movegen, AlphaBeta window, int16_t depthUnits, uint8_t nextPly);
    int32_t PrincipalVariationQuiesce(BoardState& state, History& history, const MoveStream& movegen, AlphaBeta window, uint8_t nextPly);
    void UpdateHistory(const BoardState& state, Move move, uint8_t depth);
    void UpdateKillers(Move move, uint8_t ply);
    void Reset();

private:
    // Metrics
    uint8_t m_BookMoves{0};

    // Telemetry
    // TODO: Make these all atomic if multithreading
    uint8_t m_LastSearchDepth{0};
    uint64_t m_LastNodesSearched{0};
    uint64_t m_LastNodesLookedUp{0};
    uint64_t m_LastNodesQuiesced{0};
    uint64_t m_LastSearchTime{0};
    uint64_t m_LastPawnsLookedUp{0};
    uint64_t m_LastEvaluations{0};

    float m_TimeControlSeconds{0.0};
    float m_TimeControlIncrement{0.0};
    bool m_SearchAborted{false};
    bool m_InOpeningBook{true};
    TimePoint m_SoftSearchBound;
    TimePoint m_HardSearchBound; // TODO: Better search extensions / reductions

    // These can all be shared between processors and stored in the searcher directly
    ExecutionTimer m_Timer;
    RomuMonoRandom m_FastRNG{RomuMonoRandom(time(nullptr))};
    TranspositionTable m_TranspositionTable{TranspositionTable()}; // Lock free
    AttackTable m_AttackTable{AttackTable()};
    OpeningBook m_OpeningBook{OpeningBook()};
    const Zobrist& m_Zobrist;
    const PieceSquareTables &m_PieceSquareTables;
    Evaluator m_Eval{Evaluator(m_AttackTable)};

    // Should these be associated with each "node"?
    //  - Will we need a set of these per split point as they are used for move ordering?
    //  - I should look into how these are handled, maybe I will disable these for the initial impl
    KillerMoveBuffer m_Killers{};
    HistoryTable m_HistoryTable{};
};
