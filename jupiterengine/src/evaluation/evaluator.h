#pragma once 

#include "evaluation/maskTable.h"
#include "evaluation/pawnTable.h"
#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "evaluation/distanceTable.h"
#include <cstdint>

const int32_t MATE_EVAL = 100'000'000;
const int32_t MATE_THRESHOLD = 99'000'000;
const int32_t INFINITY_EVAL = 101'000'000;

using PieceCounts = uint8_t[Color::MAX_ENUM][Piece::MAX_ENUM - 1];

struct PositionData {
    PieceCounts counts{};
    Bitboard kingBits[Color::MAX_ENUM]{0};
    uint8_t kingIndices[Color::MAX_ENUM]{0};
    PawnStructure pawns{};
    float phase{0};
};

class Evaluator {
public:
    Evaluator(const AttackTable& attackTable)
        : m_AttackTable{std::forward<const AttackTable>(attackTable)} {}
    int32_t Evaluate(const BoardState& state);
    float GamePhase(const BoardState& state) const;
    int32_t SEE(const BoardState& state, Move move) const;
    uint64_t GetEvaluationCount() const { return m_Evaluations; }
    uint64_t GetPawnTableHitCount() const { return m_PawnTableHits; }
    uint64_t GetPawnTableSize() const { return m_PawnTable.OccupancyBytes(); }
    void ComputePawnStructure(const BoardState& state, PositionData& data) const; // TODO: Make private

private:
    void CountPieces(const BoardState& state, PositionData& data) const;
    void FindKings(const BoardState& state, PositionData& data) const;
    bool IsMaterialDraw(const PositionData& data) const;
    // void ComputePawnStructure(const BoardState& state, PositionData& data) const;

    int32_t KingPawnTropism(const BoardState& state, const PositionData& data) const;
    int32_t PawnShield(const BoardState& state, const PositionData& data) const;
    int32_t PawnStorm(const BoardState& state, const PositionData& data) const;
    int32_t OpenFiles(const BoardState& state, const PositionData& data) const;
    int32_t IndependentPawnStructure(const BoardState& state, const PositionData& data) const;
    int32_t MaterialBalance(const BoardState& state, const PositionData& data) const;
    int32_t PiecePositions(const BoardState& state, const PositionData& data) const;
    int32_t Mopup(const BoardState& state, const PositionData& data) const;
    int32_t KingMobility(const BoardState& state, const PositionData& data) const;
    int32_t Mobility(const BoardState& state, const PositionData& data) const;
    uint8_t PawnChainLength(const BoardState& state, Direction::Value direction, uint8_t index) const;

private:
    uint64_t m_PawnTableHits{0};
    uint64_t m_Evaluations{0};

    DistanceTable m_DistanceTable{DistanceTable()};
    PawnTable m_PawnTable{PawnTable()};
    const AttackTable &m_AttackTable;
    MaskTable m_MaskTable{MaskTable()};
};
