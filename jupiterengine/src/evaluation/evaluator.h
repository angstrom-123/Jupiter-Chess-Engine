#pragma once 

#include "evaluation/maskTable.h"
#include "evaluation/pawnTable.h"
#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "evaluation/distanceTable.h"
#include <cstdint>
#include <cstring>

const int32_t MATE_EVAL = 100'000'000;
const int32_t MATE_THRESHOLD = 99'000'000;
const int32_t INFINITY_EVAL = 101'000'000;

constexpr uint8_t QUEEN_PHASE = 4;
constexpr uint8_t ROOK_PHASE = 2;
constexpr uint8_t BISHOP_PHASE = 1;
constexpr uint8_t KNIGHT_PHASE = 1;
constexpr uint8_t MAX_PHASE = (QUEEN_PHASE * 2) + (ROOK_PHASE * 4) + (BISHOP_PHASE * 4) + (KNIGHT_PHASE * 4);
constexpr float LATEGAME_PHASE = static_cast<float>(MAX_PHASE - QUEEN_PHASE - ROOK_PHASE * 2) / MAX_PHASE;
constexpr float EARLYGAME_PHASE = static_cast<float>((4 * BISHOP_PHASE) + (4 * KNIGHT_PHASE)) / MAX_PHASE;

struct PawnKind { 
    typedef enum : uint8_t { 
        REGULAR, 
        WEAK, 
        PASSED, 
        MAX_ENUM 
    } Value; 
};

struct PositionData {
    uint8_t counts[Color::MAX_ENUM][Piece::MAX_ENUM - 1]{};
    Bitboard kingBits[Color::MAX_ENUM]{};
    uint8_t kingIndices[Color::MAX_ENUM]{};
    PawnStructure pawns{PawnStructure{}};
    float phase{0};
};

struct EvaluatorConstants {
    float materialWeight{1.0};
    float pstWeight{1.0};
    int32_t mopupProximityFactor{4};
    int32_t mopupEdgeFactor{10};
    int32_t kingMobilityFactor{-20};
    int32_t mobilityFactor{5};
    int32_t kingPawnTropismFactors[PawnKind::MAX_ENUM]{2, 3, 6};
    int32_t missingShieldPawnFactor{-40};
    int32_t stormingPawnFactor{-15};
    int32_t sliderOpenFileFactor{45};
    int32_t weakPawnFactor{-25};
    int32_t connectedPawnFactor{15};

    static consteval EvaluatorConstants Tuned()
    {
        return EvaluatorConstants {
            .materialWeight = 2.233424259342581,
            .pstWeight = 1.931925026610482,
            .mopupProximityFactor = 7,
            .mopupEdgeFactor = 3,
            .kingMobilityFactor = -56,
            .mobilityFactor = 3,
            .kingPawnTropismFactors = { -1, 2, 17 },
            .missingShieldPawnFactor = -45,
            .stormingPawnFactor = -4,
            .sliderOpenFileFactor = 6,
            .weakPawnFactor = -11,
            .connectedPawnFactor = 24,
        };
    }
};

class Evaluator {
public:
    Evaluator(const AttackTable& attackTable)
        : m_AttackTable{attackTable} {}
    int32_t Evaluate(const BoardState& state);
    int32_t SEE(const BoardState& state, Move move) const;
    int32_t SEENew(const BoardState& state, Move move) const;
    uint64_t GetEvaluationCount() const { return m_Evaluations; }
    uint64_t GetPawnTableHitCount() const { return m_PawnTableHits; }
    uint64_t GetPawnTableSize() const { return m_PawnTable.OccupancyBytes(); }
    void SetWeights(const EvaluatorConstants& weights) { m_Constants = weights; }
    EvaluatorConstants GetWeights() const { return m_Constants; }

private:
    void ComputeGamePhase(const BoardState& state, PositionData& data) const;
    void CountPieces(const BoardState& state, PositionData& data) const;
    void FindKings(const BoardState& state, PositionData& data) const;
    void ComputePawnStructure(const BoardState& state, PositionData& data) const;

    bool IsMaterialDraw(const PositionData& data) const;
    uint8_t PawnChainLength(const BoardState& state, Direction::Value direction, uint8_t index) const;

    int32_t KingPawnTropism(const BoardState& state, const PositionData& data) const;
    int32_t PawnShield(const BoardState& state, const PositionData& data) const;
    int32_t PawnStorm(const BoardState& state, const PositionData& data) const;
    int32_t OpenFiles(const BoardState& state, const PositionData& data) const;
    int32_t IndependentPawnStructure(const BoardState& state, const PositionData& data) const;
    int32_t MaterialBalance(const BoardState& state, const PositionData& data) const;
    int32_t PiecePositions(const BoardState& state, const PositionData& data) const;
    int32_t Mopup(const BoardState& state, const PositionData& data) const;
    int32_t KingMobility(const BoardState& state, const PositionData& data) const;
    int32_t Mobility(const BoardState& state) const;

private:
    EvaluatorConstants m_Constants{EvaluatorConstants::Tuned()};

    uint64_t m_PawnTableHits{0};
    uint64_t m_Evaluations{0};
    PawnTable m_PawnTable{PawnTable()};
    const AttackTable &m_AttackTable;
    DistanceTable m_DistanceTable{DistanceTable()};
    MaskTable m_MaskTable{MaskTable()};
};
