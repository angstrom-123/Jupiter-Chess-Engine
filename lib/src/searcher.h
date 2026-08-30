#pragma once

#include "boardState.h"
#include "buffer.h"
#include "core.h"
#include "evaluator.h"
#include "executionTimer.h"
#include "history.h"
#include "move.h"
#include "attackTable.h"
#include "openingBook.h"
#include "rng.h"
#include "transpositionTable.h"

using LineBuffer = Buffer<Move, 32>;

class Searcher {
public:
    Searcher(Zobrist& zobrist, OpeningBook& openingBook);
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
    void SavePrincipalVariation(BoardState& state, Move firstMove, uint8_t depth);
    bool IsCheckmate(const BoardState& state);
    uint64_t CalculateSearchTime(const BoardState& state, uint64_t msRemaining);
    Move PickOpeningMove(const BoardState& state);
    void UnmakeMove(BoardState& state, MoveData moveData);
    int64_t Search(BoardState& state, History& history, ExecutionTimer timer, int64_t alpha, int64_t beta, uint8_t depth, uint8_t ply, uint64_t targetMs);
    int64_t Quiesce(BoardState& state, History& history, ExecutionTimer timer, int64_t alpha, int64_t beta, uint8_t ply, uint64_t targetMs);
    bool SquareUnderAttack(const BoardState& state, uint64_t bit, Color::Value color);
    bool WasLegal(const BoardState& state, MoveData moveData);

private:
    uint64_t m_TimeControlSeconds{0};
    uint64_t m_TimeControlIncrement{0};
    bool m_SearchAborted{false};
    bool m_InOpeningBook{true};
    RomuMonoRandom m_FastRNG{RomuMonoRandom(time(nullptr))};
    AttackTable m_AttackTable{AttackTable()};
    Evaluator m_Eval{Evaluator(m_AttackTable)};
    Zobrist m_Zobrist;
    TranspositionTable m_TranspositionTable;
    OpeningBook m_OpeningBook;
};
