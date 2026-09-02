#pragma once 

#include "attackTable.h"
#include "boardState.h"
#include "distanceTable.h"
#include "move.h"
#include "pieceSquareTable.h"
#include <cstdint>

const int64_t MATE_EVAL = 100'000'000;
const int64_t MATE_THRESHOLD = 99'000'000;

class Evaluator {
public:
    Evaluator(const AttackTable& attackTable)
        : m_AttackTable{std::forward<const AttackTable>(attackTable)} {}
    int64_t Evaluate(const BoardState& state) const;
    float GamePhase(const BoardState& state) const;
    int64_t SEE(const BoardState& state, Move move) const;

private:
    int64_t MaterialBalance(const BoardState& state) const;
    int64_t PiecePositions(const BoardState& state, float phase) const;
    int64_t Mopup(const BoardState& state, int64_t materialBalance, float phase) const;
    int64_t KingSafety(const BoardState& state, float phase) const;
    int64_t Mobility(const BoardState& state) const;
    int64_t PawnStructure(const BoardState& state) const;

private:
    AttackTable m_AttackTable;
    DistanceTable m_DistanceTable{DistanceTable()};
    PieceSquareTables m_PieceSquareTables{PieceSquareTables()};
};
