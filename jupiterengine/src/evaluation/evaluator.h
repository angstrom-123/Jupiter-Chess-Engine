#pragma once 

#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "evaluation/distanceTable.h"
#include <cstdint>

const int32_t MATE_EVAL = 100'000'000;
const int32_t MATE_THRESHOLD = 99'000'000;
const int32_t INFINITY_EVAL = 101'000'000;

class Evaluator {
public:
    Evaluator(const AttackTable& attackTable)
        : m_AttackTable{std::forward<const AttackTable>(attackTable)} {}
    int32_t Evaluate(const BoardState& state) const;
    float GamePhase(const BoardState& state) const;
    int32_t SEE(const BoardState& state, Move move) const;

private:
    int32_t MaterialBalance(const BoardState& state, int64_t (& pieceCounts)[Color::MAX_ENUM][Piece::MAX_ENUM], bool& isMaterialDraw) const;
    int32_t PiecePositions(const BoardState& state, float phase) const;
    int32_t Mopup(const BoardState& state, int32_t materialBalance, float phase) const;
    int32_t KingSafety(const BoardState& state, float phase) const;
    int32_t Mobility(const BoardState& state) const;
    int32_t PawnStructure(const BoardState& state) const;

private:
    DistanceTable m_DistanceTable{DistanceTable()};
    const AttackTable &m_AttackTable;
};
