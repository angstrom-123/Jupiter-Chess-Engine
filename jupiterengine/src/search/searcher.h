#pragma once

#include "board/boardState.h"
#include "board/history.h"
#include "datastructure/buffer.h"
#include "evaluation/evaluator.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"
#include "movegen/attackTable.h"
#include "movegen/moveStream.h"
#include "util/executionTimer.h"
#include "util/rng.h"
#include "search/openingBook.h"
#include "search/transpositionTable.h"

using LineBuffer = Buffer<Move, MAX_PLY>;

constexpr uint16_t PLY_UNIT = 64;
constexpr uint16_t HALF_PLY_UNIT = PLY_UNIT / 2;

class Searcher {
public:
    Searcher(Zobrist& zobrist, PieceSquareTables& pieceSquareTables);
    Move FindBest(BoardState& state, History& history, uint64_t msRemaining);
    void SetTimeControl(uint64_t seconds, uint64_t increment);
    void TelemetryJSON(std::string& result) const;
    void MetricsJSON(std::string& result) const;

private:
    void SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth, LineBuffer& pv);
    bool IsCheckmate(const BoardState& state);
    void CalculateSearchTime(uint64_t msRemaining);
    Move PickOpeningMove(const BoardState& state);
    int32_t Search(BoardState& state, History& history, int32_t alpha, int32_t beta, int16_t depthUnits, uint8_t ply);
    int32_t Quiesce(BoardState& state, History& history, int32_t alpha, int32_t beta, uint8_t ply);

private:
    // Metrics
    uint8_t m_BookMoves{0};

    // Telemetry
    uint8_t m_LastSearchDepth{0};
    uint64_t m_LastNodesSearched{0};
    uint64_t m_LastNodesLookedUp{0};
    uint64_t m_LastNodesQuiesced{0};
    uint64_t m_LastSearchTime{0};
    uint64_t m_LastPawnsLookedUp{0};
    uint64_t m_LastEvaluations{0};

    uint64_t m_TimeControlSeconds{0};
    uint64_t m_TimeControlIncrement{0};
    bool m_SearchAborted{false};
    bool m_InOpeningBook{true};
    uint64_t m_SoftSearchBound{0};
    uint64_t m_HardSearchBound{0}; // TODO: Better search extensions / reductions

    ExecutionTimer m_Timer;
    Buffer<KillerMoveBuffer, MAX_PLY> m_Killers;
    RomuMonoRandom m_FastRNG{RomuMonoRandom(time(nullptr))};
    TranspositionTable m_TranspositionTable{TranspositionTable()};
    AttackTable m_AttackTable{AttackTable()};
    OpeningBook m_OpeningBook{OpeningBook()};
    const Zobrist& m_Zobrist;
    const PieceSquareTables &m_PieceSquareTables;
    uint16_t m_HistoryTable[64][64]{};
    Evaluator m_Eval{Evaluator(m_AttackTable)};
};
