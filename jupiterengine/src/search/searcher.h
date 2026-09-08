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

private:
    void SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth, LineBuffer& pv);
    bool IsCheckmate(const BoardState& state);
    void CalculateSearchTime(uint64_t msRemaining);
    Move PickOpeningMove(const BoardState& state);
    int32_t Search(BoardState& state, History& history, int32_t alpha, int32_t beta, int16_t depthUnits, uint8_t ply);
    int32_t Quiesce(BoardState& state, History& history, int32_t alpha, int32_t beta, uint8_t ply);

public:
    // Metrics
    uint8_t bookMoves{0};
    uint64_t ttSize{0};
    uint64_t tablebasePrunes{0};

    // Telemetry
    uint8_t searchDepth{0};
    uint64_t nodesSearched{0};
    uint64_t nodesLookedUp{0};
    uint64_t nodesQuiesced{0};
    uint64_t searchTime{0};

private:
    uint64_t m_TimeControlSeconds{0};
    uint64_t m_TimeControlIncrement{0};
    bool m_SearchAborted{false};
    bool m_InOpeningBook{true};
    uint64_t m_SoftSearchBound{0};
    uint64_t m_HardSearchBound{0};
    ExecutionTimer m_Timer;
    Buffer<KillerMoveBuffer, MAX_PLY> m_Killers;
    RomuMonoRandom m_FastRNG{RomuMonoRandom(time(nullptr))};
    AttackTable m_AttackTable{AttackTable()};
    OpeningBook m_OpeningBook{OpeningBook()};
    Evaluator m_Eval{Evaluator(m_AttackTable)};
    const Zobrist& m_Zobrist;
    const PieceSquareTables &m_PieceSquareTables;
    TranspositionTable m_TranspositionTable;
    uint16_t m_HistoryTable[64][64]{};
};
