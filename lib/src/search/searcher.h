#pragma once

#include "board/boardState.h"
#include "board/history.h"
#include "core.h"
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
    Searcher(Zobrist& zobrist, OpeningBook& openingBook, PieceSquareTables& pieceSquareTables);
    Move FindBest(BoardState& state, History& history, uint64_t msRemaining);
    MoveData MakeMove(BoardState &state, Move move);
    void SetTimeControl(uint64_t seconds, uint64_t increment);

public:
    // Metrics
    uint8_t bookMoves{0};
    uint64_t ttSize{0};

    // Telemetry
    uint8_t searchDepth{0};
    uint64_t nodesSearched{0};
    uint64_t nodesLookedUp{0};
    uint64_t nodesQuiesced{0};
    uint64_t searchTime{0};

private:
    void SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth, LineBuffer& pv);
    bool IsCheckmate(const BoardState& state);
    void CalculateSearchTime(ExecutionTimer timer, uint64_t msRemaining);
    Move PickOpeningMove(const BoardState& state);
    void UnmakeMove(BoardState& state, MoveData moveData);
    int64_t Search(BoardState& state, History& history, ExecutionTimer timer, int64_t alpha, int64_t beta, int16_t depthUnits, uint8_t ply);
    int64_t Quiesce(BoardState& state, History& history, ExecutionTimer timer, int64_t alpha, int64_t beta, uint8_t ply);
    bool SquareUnderAttack(const BoardState& state, uint64_t bit, Color::Value color);
    bool WasLegal(const BoardState& state, MoveData moveData);
    bool IsCheck(const BoardState& state);

private:
    uint64_t m_TimeControlSeconds{0};
    uint64_t m_TimeControlIncrement{0};
    bool m_SearchAborted{false};
    bool m_InOpeningBook{true};
    uint64_t m_SoftSearchBound{0};
    uint64_t m_HardSearchBound{0};
    Buffer<KillerMoveBuffer, MAX_PLY> m_Killers;
    RomuMonoRandom m_FastRNG{RomuMonoRandom(time(nullptr))};
    AttackTable m_AttackTable{AttackTable()};
    Evaluator m_Eval{Evaluator(m_AttackTable)};
    const Zobrist &m_Zobrist;
    const OpeningBook &m_OpeningBook;
    const PieceSquareTables &m_PieceSquareTables;
    TranspositionTable m_TranspositionTable;
    uint16_t m_HistoryTable[64][64]{};
};
