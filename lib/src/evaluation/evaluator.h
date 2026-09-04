#pragma once 

#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "evaluation/distanceTable.h"
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
    int64_t MaterialBalance(const BoardState& state, int64_t (& pieceCounts)[Color::MAX_ENUM][Piece::MAX_ENUM], bool& isMaterialDraw) const;
    int64_t PiecePositions(const BoardState& state, float phase) const;
    int64_t Mopup(const BoardState& state, int64_t materialBalance, float phase) const;
    int64_t KingSafety(const BoardState& state, float phase) const;
    int64_t Mobility(const BoardState& state) const;
    int64_t PawnStructure(const BoardState& state) const;

private:
    DistanceTable m_DistanceTable{DistanceTable()};
    const AttackTable &m_AttackTable;
};
