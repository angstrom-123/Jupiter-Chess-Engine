#pragma once 

#include "attackTable.h"
#include "boardState.h"
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
    uint8_t GamePhase(const BoardState& state) const;
    int64_t SEE(const BoardState& state, Move move) const;

private:
    int64_t MaterialBalance(const BoardState& state) const;
    int64_t PiecePositions(const BoardState& state) const;
    int64_t Mobility(const BoardState& state, const AttackTable& table) const;

private:
    AttackTable m_AttackTable;
    PieceSquareTables m_PieceSquareTables{PieceSquareTables()};
};
