#include "evaluation/evaluator.h"
#include "board/bitboard.h"
#include "core.h"
#include "datastructure/buffer.h"
#include "evaluation/pawnTable.h"
#include "util/instrumenter.h"
#include "movegen/movegen.h"

#include <bit>
#include <cmath>
#include <cstdint>

struct PositionData {
    uint8_t counts[Color::MAX_ENUM][Piece::MAX_ENUM - 1]{};
    Bitboard kingBits[Color::MAX_ENUM]{};
    uint8_t kingIndices[Color::MAX_ENUM]{};
    PawnStructure pawns{};
    float phase{0.0};
};

int32_t Evaluator::Evaluate(const BoardState& state)
{
    JUPITER_TRACE();

    m_Evaluations++;

    // Draw by fifty move rule
    if (state.fiftyMoveCounter >= 100)
        return 0;

    PositionData data;
    CountPieces(std::forward<const BoardState>(state), data);
    
    if (IsMaterialDraw(data))
        return 0;

    data.phase = GamePhase(std::forward<const BoardState>(state));
    FindKings(std::forward<const BoardState>(state), data);

    int32_t eval = 0;

    PTEntry pte;
    if ((pte = m_PawnTable.Get(state.pawnKey)).IsValid()) {
        // Lookup pawn structure and score
        m_PawnTableHits++;
        data.pawns = pte.structure;
        eval += pte.score;
    } else {
        // Evaluate from scratch and save to pawn table
        ComputePawnStructure(std::forward<const BoardState>(state), data);
        int32_t score = IndependentPawnStructure(std::forward<const BoardState>(state), data);
        m_PawnTable.Save(state.pawnKey, score, data.pawns);
        eval += score;
    }

    eval += MaterialBalance(std::forward<const BoardState>(state), data);
    eval += PiecePositions(std::forward<const BoardState>(state), data);
    eval += Mobility(std::forward<const BoardState>(state), data);
    eval += Mopup(std::forward<const BoardState>(state), data);
    eval += KingMobility(std::forward<const BoardState>(state), data);
    eval += KingPawnTropism(std::forward<const BoardState>(state), data);
    eval += PawnShield(std::forward<const BoardState>(state), data);
    eval += PawnStorm(std::forward<const BoardState>(state), data);
    eval += OpenFiles(std::forward<const BoardState>(state), data);

    return eval;
}

// Since this function already needs to count all pieces, can check for material draw here
int32_t Evaluator::MaterialBalance(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    int32_t materialEval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    int64_t nPawns = data.counts[friendly][Piece::PAWN] - data.counts[enemy][Piece::PAWN];
    int64_t nKnights = data.counts[friendly][Piece::KNIGHT] - data.counts[enemy][Piece::KNIGHT];
    int64_t nBishops = data.counts[friendly][Piece::BISHOP] - data.counts[enemy][Piece::BISHOP];
    int64_t nRooks = data.counts[friendly][Piece::ROOK] - data.counts[enemy][Piece::ROOK];
    int64_t nQueens = data.counts[friendly][Piece::QUEEN] - data.counts[enemy][Piece::QUEEN];

    materialEval = (nPawns * Piece::Evaluate(Piece::PAWN)) 
        + (nKnights * Piece::Evaluate(Piece::KNIGHT)) 
        + (nBishops * Piece::Evaluate(Piece::BISHOP)) 
        + (nRooks * Piece::Evaluate(Piece::ROOK)) 
        + (nQueens * Piece::Evaluate(Piece::QUEEN));

    return materialEval * m_Constants.materialWeight;
}

int32_t Evaluator::PiecePositions(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    PSTScore relativeScore = (state.turn == Color::WHITE) ? state.pstScore : -state.pstScore;
    return ((relativeScore.midgame * (1.0 - data.phase)) + (relativeScore.endgame * data.phase)) * m_Constants.pstWeight;
}

int32_t Evaluator::Mopup(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    int32_t mopupEval = 0;

    // Only mopup if endgame
    const float MIN_PHASE = 0.7;
    if (data.phase < MIN_PHASE)
        return 0;

    uint8_t friendlyKing = data.kingIndices[state.turn];
    uint8_t enemyKing = data.kingIndices[Color::Opposite(state.turn)];

    // Bonus for king-king proximity
    mopupEval += (14 - m_DistanceTable.Manhattan(friendlyKing, enemyKing)) * m_Constants.mopupProximityFactor;

    // Bonus for enemy king proximity to edge
    mopupEval += m_DistanceTable.ManhattanFromCenter(enemyKing) * m_Constants.mopupEdgeFactor;

    // Scale by endgame weight
    return mopupEval * (data.phase - MIN_PHASE);
}

int32_t Evaluator::KingMobility(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    int32_t kingMobilityEval = 0;

    const float MAX_PHASE = 0.7;
    if (data.phase > MAX_PHASE)
        return 0;
    
    Movegen movegen(std::forward<const BoardState>(state), std::forward<const AttackTable>(m_AttackTable));
    uint8_t kingIndex = data.kingIndices[state.turn];

    // Imagine a friendly queen where the king is and see how much it can move
    AttackMoveBuffer attacks;
    movegen.FindQueenAttacks(kingIndex, attacks);

    // Penalize excessive mobility (more than 3 squares)
    if (attacks.Size() > 3)
        kingMobilityEval += (attacks.Size() - 3) * m_Constants.kingMobilityFactor;

    return kingMobilityEval * (MAX_PHASE - data.phase);
}

int32_t Evaluator::Mobility(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    (void) data;

    Movegen movegen(std::forward<const BoardState>(state), std::forward<const AttackTable>(m_AttackTable));
    return (movegen.CountAllAttacks() + movegen.CountAllQuiets()) * m_Constants.mobilityFactor;
}

int32_t Evaluator::KingPawnTropism(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    const PawnStructure::Relative& rel = data.pawns.relative[state.turn];

    uint8_t distances[PawnKind::MAX_ENUM] = { 0, 0, 0}; // Manhattan (total)
    uint8_t weightCount = 0;

    // NOTE: Using 14-manhattan here to boost score for proximity rather than distance
    uint8_t kingIndex = data.kingIndices[state.turn];
    const auto SumDistances = [this, &distances, &weightCount, kingIndex](Bitboard bb, PawnKind::Value kind) {
        while (bb) {
            distances[kind] += (14 - m_DistanceTable.Manhattan(std::countr_zero(bb), kingIndex));
            weightCount++;
            bb &= (bb - 1);
        }
    };

    SumDistances(state.pieces.Occupancy(state.turn, Piece::PAWN) & ~(rel.weak | rel.passed), PawnKind::REGULAR);
    SumDistances(rel.weak, PawnKind::WEAK);
    SumDistances(rel.passed, PawnKind::PASSED);

    // Avoid division by 0 if there are no pawns
    if (weightCount == 0)
        return 0;

    int32_t kpTropismEval = distances[PawnKind::REGULAR] * m_Constants.kingPawnTropismFactors[PawnKind::REGULAR]
        + distances[PawnKind::WEAK] * m_Constants.kingPawnTropismFactors[PawnKind::WEAK]
        + distances[PawnKind::PASSED] * m_Constants.kingPawnTropismFactors[PawnKind::PASSED];
    kpTropismEval /= weightCount;

    // More important in endgame, scale up
    return kpTropismEval * std::max(data.phase, 0.6f);
}

int32_t Evaluator::PawnShield(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    int32_t shieldEval = 0;

    const float MAX_PHASE = 0.7;
    if (data.phase > MAX_PHASE)
        return 0.0;

    const PawnStructure::Relative& rel = data.pawns.relative[state.turn];
    uint8_t kingFile = data.kingIndices[state.turn] & 7;
    uint8_t kingRank = data.kingIndices[state.turn] / 8;

    if ((state.turn == Color::WHITE && kingRank > 4) || (state.turn == Color::BLACK && kingRank < 3)) {
        // Shield can move up with the king
        uint8_t shieldRank = (2 * state.turn) + kingRank - 1;

        // Only evaluate shield on the flanks
        uint8_t missingPawns = 0;
        if (kingFile < 3) {
            // Pawns on the 0, 1, 2 files need to be at shield rank
            if (rel.furthest.Rank(0) != shieldRank) missingPawns++;
            if (rel.furthest.Rank(1) != shieldRank) missingPawns++;
            if (rel.furthest.Rank(2) != shieldRank) missingPawns++;
        } else if (kingFile > 4) {
            // Pawns on the 5, 6, 7 files need to be at shield rank
            if (rel.furthest.Rank(5) != shieldRank) missingPawns++;
            if (rel.furthest.Rank(6) != shieldRank) missingPawns++;
            if (rel.furthest.Rank(7) != shieldRank) missingPawns++;
        }

        shieldEval += missingPawns * m_Constants.missingShieldPawnFactor;
    }

    // More important in middlegame
    return shieldEval * (MAX_PHASE - data.phase);
}

int32_t Evaluator::PawnStorm(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    const float MAX_PHASE = 0.7;
    if (data.phase > MAX_PHASE)
        return 0.0;

    const PawnStructure::Relative& opp = data.pawns.relative[Color::Opposite(state.turn)];
    uint8_t kingIndex = data.kingIndices[state.turn];
    uint8_t kingFile = kingIndex & 7;
    
    int32_t stormEval = 0;

    // Count average distance to opponent pawns in current and adjacent files
    uint8_t distance = 23; // 8 + 7 + 8
    uint8_t rank0 = opp.furthest.Rank(kingFile);
    uint8_t rank1 = (kingFile > 0) ? opp.furthest.Rank(kingFile - 1) : UINT8_MAX;
    uint8_t rank2 = (kingFile < 7) ? opp.furthest.Rank(kingFile + 1) : UINT8_MAX;

    if (rank0 < UINT8_MAX) distance -= m_DistanceTable.Manhattan(kingIndex, rank0);
    if (rank1 < UINT8_MAX) distance -= m_DistanceTable.Manhattan(kingIndex, rank1);
    if (rank2 < UINT8_MAX) distance -= m_DistanceTable.Manhattan(kingIndex, rank2);

    uint8_t proximity = 23 - distance;
    stormEval += proximity * m_Constants.stormingPawnFactor;

    return stormEval * (MAX_PHASE - data.phase);
}

int32_t Evaluator::OpenFiles(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    int32_t openEval = 0;

    Bitboard openQueens = data.pawns.openFiles & state.pieces.Occupancy(state.turn, Piece::QUEEN);
    Bitboard openRooks = data.pawns.openFiles & state.pieces.Occupancy(state.turn, Piece::ROOK);

    uint8_t openCount = std::popcount(openQueens) + std::popcount(openRooks);
    openEval += openCount * m_Constants.sliderOpenFileFactor;

    return openEval;

    // TODO: Penalty for open files next to king
}

// This is the expensive one that gets its eval cached in the pawn hash table
int32_t Evaluator::IndependentPawnStructure(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    int32_t structureEval = 0;

    // Weak pawns
    structureEval += std::popcount(data.pawns.relative[state.turn].weak) * m_Constants.weakPawnFactor;

    // Connected pawns
    {
        Bitboard pawns = state.pieces.Occupancy(state.turn, Piece::PAWN);
        while (pawns) {
            uint8_t index = std::countr_zero(pawns);
            uint8_t chainLeft = PawnChainLength(state, Direction::LEFT, index);
            uint8_t chainRight = PawnChainLength(state, Direction::RIGHT, index);
            structureEval += (chainLeft + chainRight) * m_Constants.connectedPawnFactor;
            pawns &= (pawns - 1);
        };
    }

    // TODO: More stuff in here

    return structureEval;
}

// 0.0 - 1.0 (midgame - endgame)
float Evaluator::GamePhase(const BoardState& state) const
{
    JUPITER_TRACE();

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    constexpr int64_t QUEEN_PHASE = 4;
    constexpr int64_t ROOK_PHASE = 2;
    constexpr int64_t BISHOP_PHASE = 1;
    constexpr int64_t KNIGHT_PHASE = 1;

    constexpr int64_t MAX_PHASE = (QUEEN_PHASE * 2) + (ROOK_PHASE * 4) + (BISHOP_PHASE * 4) + (KNIGHT_PHASE * 4);

    int64_t nQueens = state.pieces.Count(friendly, Piece::QUEEN) - state.pieces.Count(enemy, Piece::QUEEN);
    int64_t nRooks = state.pieces.Count(friendly, Piece::ROOK) - state.pieces.Count(enemy, Piece::ROOK);
    int64_t nBishops = state.pieces.Count(friendly, Piece::BISHOP) - state.pieces.Count(enemy, Piece::BISHOP);
    int64_t nKnights = state.pieces.Count(friendly, Piece::KNIGHT) - state.pieces.Count(enemy, Piece::KNIGHT);

    int64_t currentPhase = (QUEEN_PHASE * nQueens) + (ROOK_PHASE * nRooks) + (BISHOP_PHASE * nBishops) + (KNIGHT_PHASE * nKnights);
    currentPhase = std::min(currentPhase, MAX_PHASE); // Clamp in case of promotions

    return 1.0 - (static_cast<float>(currentPhase) / static_cast<float>(MAX_PHASE));
}

int32_t Evaluator::SEE(const BoardState& state, Move move) const
{
    JUPITER_TRACE();

    BitboardSet pieces(state.pieces);
    Color::Value enemy = Color::Opposite(state.turn);

    // Simulate first capture
    Piece::Value firstCapture = pieces.PieceInSquare(enemy, move.to);
    if (!Piece::IsValid(firstCapture)) // en passant
        firstCapture = pieces.PieceInSquare(enemy, (enemy == Color::WHITE) ? move.to - 8 : move.to + 8);

    pieces.Unset(enemy, firstCapture, move.to);
    pieces.Unset(state.turn, move.piece, move.from);
    pieces.Set(state.turn, move.piece, move.to);

    Buffer<int32_t, 16> gain;
    gain.PushBack(Piece::Evaluate(firstCapture));

    // TODO: Don't recalculate attackers at each iteration, just update the bitboards 
    //       iteratively at each step. Then only sliders need recalculation (in case of discovery).

    Color::Value turn = enemy;
    Piece::Value target = move.piece;
    while (true) {
        Color::Value opponentTurn = Color::Opposite(turn);

        // Find attackers
        Bitboard attackers[Color::MAX_ENUM][Piece::MAX_ENUM];
        for (const Color::Value friendly : { Color::WHITE, Color::BLACK }) {
            Color::Value opponent = Color::Opposite(friendly);

            // Had to unroll the loop due to specialised functions being used per piece type
            attackers[friendly][Piece::PAWN]   = m_AttackTable.PawnAttacks(move.to, opponent) & pieces.Occupancy(friendly, Piece::PAWN);
            attackers[friendly][Piece::KNIGHT] = m_AttackTable.KnightAttacks(move.to) & pieces.Occupancy(friendly, Piece::KNIGHT);
            attackers[friendly][Piece::BISHOP] = m_AttackTable.BishopAttacks(move.to, pieces.Occupancy()) & pieces.Occupancy(friendly, Piece::BISHOP);
            attackers[friendly][Piece::ROOK]   = m_AttackTable.RookAttacks(move.to, pieces.Occupancy()) & pieces.Occupancy(friendly, Piece::ROOK);
            attackers[friendly][Piece::QUEEN]  = m_AttackTable.QueenAttacks(move.to, pieces.Occupancy()) & pieces.Occupancy(friendly, Piece::QUEEN);
            attackers[friendly][Piece::KING]   = m_AttackTable.KingAttacks(move.to) & pieces.Occupancy(friendly, Piece::KING);
        }

        // Find least valuable attacker
        uint8_t from = UINT8_MAX;
        Piece::Value attacker = Piece::Invalid();
        for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
            Bitboard attackerOccupancy = attackers[turn][piece];
            if (attackerOccupancy) {
                from = std::countr_zero(attackerOccupancy);
                attacker = piece;
                attackers[turn][piece] &= (attackerOccupancy - 1);
                break;
            }
        }
        if (!Piece::IsValid(attacker))
            break;

        // Update gain
        gain.PushBack(Piece::Evaluate(target) - gain[gain.Size() - 1]);

        // Simulate capture
        pieces.Unset(turn, attacker, from);
        pieces.Unset(opponentTurn, target, move.to);
        pieces.Set(turn, attacker, move.to);

        // Swap turn
        target = attacker;
        turn = Color::Opposite(turn);
    }

    // Traverse gain
    for (int8_t i = static_cast<int8_t>(gain.Size()) - 1; i > 0; i--)
        gain[i - 1] = -std::max(-gain[i - 1], gain[i]);

    return gain[0];
}

void Evaluator::CountPieces(const BoardState& state, PositionData& data) const 
{
    JUPITER_TRACE();

    for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM - 1; piece++) {
        data.counts[Color::WHITE][piece] = state.pieces.Count(Color::WHITE, piece);
        data.counts[Color::BLACK][piece] = state.pieces.Count(Color::BLACK, piece);
    }
}

void Evaluator::FindKings(const BoardState& state, PositionData& data) const
{
    JUPITER_TRACE();

    data.kingBits[Color::WHITE] = state.pieces.Occupancy(Color::WHITE, Piece::KING);
    data.kingBits[Color::BLACK] = state.pieces.Occupancy(Color::BLACK, Piece::KING);
    data.kingIndices[Color::WHITE] = std::countr_zero(data.kingBits[Color::WHITE]);
    data.kingIndices[Color::BLACK] = std::countr_zero(data.kingBits[Color::BLACK]);
}

bool Evaluator::IsMaterialDraw(const PositionData& data) const
{
    JUPITER_TRACE();

    uint8_t majorCount = data.counts[Color::WHITE][Piece::PAWN] + data.counts[Color::BLACK][Piece::PAWN] 
        + data.counts[Color::WHITE][Piece::ROOK] + data.counts[Color::BLACK][Piece::ROOK]
        + data.counts[Color::WHITE][Piece::QUEEN] + data.counts[Color::BLACK][Piece::QUEEN];

    // Any amount of pawns, rooks, or queens could potentially mate
    if (majorCount > 0) 
        return false;

    uint8_t minorCount[Color::MAX_ENUM];
    minorCount[Color::WHITE] = data.counts[Color::WHITE][Piece::BISHOP] + data.counts[Color::WHITE][Piece::KNIGHT];
    minorCount[Color::BLACK] = data.counts[Color::BLACK][Piece::BISHOP] + data.counts[Color::BLACK][Piece::KNIGHT];

    // Always a draw - 1 minor piece a side is not enough
    if (minorCount[Color::WHITE] < 2 && minorCount[Color::BLACK] < 2)
        return true;

    // No forced mate with only 2 knights (other combos of 2 minors can mate)
    for (const Color::Value color : Color::values) {
        if ((minorCount[color] == 2 && data.counts[color][Piece::KNIGHT] == 2))
            return true;
    }

    return false;
}

void Evaluator::ComputePawnStructure(const BoardState& state, PositionData& data) const
{
    JUPITER_TRACE();

    PawnStructure& ps = data.pawns;

    const Bitboard FILE_MASK = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;

    // TODO: Convert most of this to use the mask table (m_MaskTable)

    // Open files
    {
        Bitboard blocked = 0;
        Bitboard pawns = state.pieces.Occupancy(Piece::PAWN);
        while (pawns) {
            uint8_t file = std::countr_zero(pawns) & 7;
            blocked |= (FILE_MASK << file);
            pawns &= (pawns - 1);
        }
        ps.openFiles = ~blocked;
    }

    for (const Color::Value color : Color::values) {
        Bitboard pawns = state.pieces.Occupancy(color, Piece::PAWN);

        // Doubled pawns
        for (uint8_t file = 0; file < 8; file++) {
            Bitboard filePawns = pawns & (FILE_MASK << file);
            uint8_t count = std::popcount(filePawns);
            for (uint8_t i = count; i > 1; i--) {
                // Count from the lowest to the highest
                uint8_t index = (color == Color::WHITE) ? 63 - std::countl_zero(filePawns) : std::countr_zero(filePawns);
                ps.relative[color].weak |= (1ull << index);
                filePawns = (color == Color::WHITE) ? filePawns & ~(1ull << index) : filePawns & (filePawns - 1);
            }
        }

        // Isolated pawns
        Bitboard isolatedPawns = pawns;
        while (isolatedPawns) {
            uint8_t index = std::countr_zero(isolatedPawns);
            uint8_t file = index & 7;

            // Save to bitboard if it has no neighbours
            if (!(m_MaskTable.NeighbouringFiles(file) & pawns))
                ps.relative[color].weak |= (1ull << index);

            isolatedPawns &= (isolatedPawns - 1);
        }

        // TODO: Backwards pawns

        // Furthest forward pawn ranks
        Bitboard furthestPawns = pawns;
        uint8_t finishedFiles = 0; // Bitmask of files where we already got the highest up pawn
        while (furthestPawns) {
            // For white use countr to find the lowest index (highest advance) furthestPawns first 
            // For black use 64 - countl to find the highest index (highest advance) furthestPawns first
            uint8_t index = (color == Color::WHITE) ? std::countr_zero(furthestPawns) : 63 - std::countl_zero(furthestPawns);
            uint8_t file = index & 7;
            if (!(finishedFiles & (1ull << file))) {
                ps.relative[color].furthest.Pack(file, index / 8);
                finishedFiles |= (1ull << file);
            }
            
            // For black we need to pop the MSB so we can't use the &= n - 1 trick
            furthestPawns = (color == Color::WHITE) ? furthestPawns & (furthestPawns - 1) : furthestPawns & ~(1ull << index);
        }

        // Passed pawns
        for (uint8_t file = 0; file < 8; file++) {
            // Passed pawns are strictly the frontmost in each rank
            uint8_t rank = ps.relative[color].furthest.Rank(file);
            if (rank == UINT8_MAX)
                continue;

            // No opponent pawns in the way means it is a passed pawn
            Bitboard opponentPawns = state.pieces.Occupancy(Color::Opposite(color), Piece::PAWN);
            if (!(opponentPawns & m_MaskTable.PassedPawnBlockers(color, 8 * rank + file)))
                ps.relative[color].passed |= (1ull << (rank * 8 + file));
        }
    }
}

uint8_t Evaluator::PawnChainLength(const BoardState& state, Direction::Value direction, uint8_t index) const 
{
    // For such a small amount of lengths, linear search is ok (especially since most will exit early)
    Bitboard pawns = state.pieces.Occupancy(state.turn, Piece::PAWN);
    for (uint8_t length = 1; length < 7; length++) {
        Bitboard chainMask = m_MaskTable.ConnectedPawns(state.turn, length, direction, index);

        // If the chain is broken at this length we return the previous length
        if ((chainMask & pawns) != chainMask)
            return length - 1;
    }

    // Maximum chain
    return 6;
}
