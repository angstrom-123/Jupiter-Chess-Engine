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

struct PawnKind { typedef enum : uint8_t { REGULAR, WEAK, PASSED, MAX_ENUM } Value; };

struct EvaluatorConstants {
    float materialWeight{1.0};
    float pstWeight{1.0};
    int32_t mopupProximityFactor{4};
    int32_t mopupEdgeFactor{10};
    int32_t kingMobilityFactor{-20};
    int32_t mobilityFactor{5};
    int32_t kingPawnTropismFactors[PawnKind::MAX_ENUM]{2, 3, 6};
    int32_t missingShieldPawnFactor{-40};
    int32_t stormingPawnFactor{-7};
    int32_t sliderOpenFileFactor{45};
    int32_t weakPawnFactor{-25};
    int32_t connectedPawnFactor{15};

    static EvaluatorConstants Tuned()
    {
        return EvaluatorConstants {
            .materialWeight = 1.7982300519943237,
            .pstWeight = 0.8320792317390442,
            .mopupProximityFactor = -4,
            .mopupEdgeFactor = 7,
            .kingMobilityFactor = -22,
            .mobilityFactor = 3,
            .kingPawnTropismFactors = { -3, 8, 5 },
            .missingShieldPawnFactor = -29,
            .stormingPawnFactor = -3,
            .sliderOpenFileFactor = 44,
            .weakPawnFactor = -23,
            .connectedPawnFactor = 25
        };
    }
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
    void Tune(const EvaluatorConstants& weights) { m_Constants = weights; }
    EvaluatorConstants GetWeights() const { return m_Constants; }

private:
    void CountPieces(const BoardState& state, struct PositionData& data) const;
    void FindKings(const BoardState& state, struct PositionData& data) const;
    bool IsMaterialDraw(const struct PositionData& data) const;
    void ComputePawnStructure(const BoardState& state, struct PositionData& data) const;
    uint8_t PawnChainLength(const BoardState& state, Direction::Value direction, uint8_t index) const;

    int32_t KingPawnTropism(const BoardState& state, const struct PositionData& data) const;
    int32_t PawnShield(const BoardState& state, const struct PositionData& data) const;
    int32_t PawnStorm(const BoardState& state, const struct PositionData& data) const;
    int32_t OpenFiles(const BoardState& state, const struct PositionData& data) const;
    int32_t IndependentPawnStructure(const BoardState& state, const struct PositionData& data) const;
    int32_t MaterialBalance(const BoardState& state, const struct PositionData& data) const;
    int32_t PiecePositions(const BoardState& state, const struct PositionData& data) const;
    int32_t Mopup(const BoardState& state, const struct PositionData& data) const;
    int32_t KingMobility(const BoardState& state, const struct PositionData& data) const;
    int32_t Mobility(const BoardState& state, const struct PositionData& data) const;

private:
    // TODO: More tuning?
    // EvaluatorConstants m_Constants{};
    EvaluatorConstants m_Constants{EvaluatorConstants::Tuned()};

    uint64_t m_PawnTableHits{0};
    uint64_t m_Evaluations{0};

    DistanceTable m_DistanceTable{DistanceTable()};
    PawnTable m_PawnTable{PawnTable()};
    const AttackTable &m_AttackTable;
    MaskTable m_MaskTable{MaskTable()};
};
