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

// TODO: Get rid of all the floats and see if the profile comes down. 
//          Register pressure and moving between fp and int registers could be VERY expensive. This 
//          is suggested by the expensive movq and vmovups instructions (13% and 20% respectively) 
//          in the profile.
//
//       Also, just reduced padding of pawn table entries 56 -> 48 bytes so we can see if that 
//          helps improve probing speed.

int32_t Evaluator::Evaluate(const BoardState& state)
{
    JUPITER_TRACE();

    m_Evaluations++;

    // Draw by fifty move rule
    if (state.fiftyMoveCounter >= 100)
        return 0;

    PositionData data;

    CountPieces(state, data);
    if (IsMaterialDraw(data))
        return 0;

    // Initial lazy evaluation (very fast to compute)
    int32_t eval = 0;

    ComputeGamePhase(state, data);
    eval += MaterialBalance(state, data);
    eval += PiecePositions(state, data);

    // TODO: This threshold should be roughly equivalent to maximum eval swing from the subsequent 
    //       evaluation factors. I just magicified 1500 but it seems to improve the engine ¯\(ツ)/¯
    constexpr int32_t LAZY_THRESHOLD = 1500;
    if (std::abs(eval) >= LAZY_THRESHOLD)
        return eval;

    // Probe the pawn hash table for a matching entry, or compute the structure ourselves
    PTEntry pte = m_PawnTable.Get(state.pawnKey);
    if (pte.IsValid()) {
        // Lookup pawn structure and score
        m_PawnTableHits++;
        data.pawns = pte.structure;
        eval += pte.score;
    } else {
        // Evaluate from scratch and save to pawn table
        ComputePawnStructure(state, data);
        int32_t score = IndependentPawnStructure(state, data);
        m_PawnTable.Save(state.pawnKey, score, data.pawns);
        eval += score;
    }

    FindKings(state, data);
    eval += Mobility(state);
    eval += Mopup(state, data);
    eval += KingMobility(state, data);
    eval += KingPawnTropism(state, data);
    eval += PawnShield(state, data);
    eval += PawnStorm(state, data);
    eval += OpenFiles(state, data);

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

    PSTScore score = (state.turn == Color::WHITE) ? state.pstScore : -state.pstScore;
    return ((score.midgame * (1.0 - data.phase)) + (score.endgame * data.phase)) * m_Constants.pstWeight;
}

int32_t Evaluator::Mopup(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    int32_t mopupEval = 0;

    // Only mopup if endgame
    if (data.phase < LATEGAME_PHASE)
        return 0;

    uint8_t friendlyKing = data.kingIndices[state.turn];
    uint8_t enemyKing = data.kingIndices[Color::Opposite(state.turn)];

    // Bonus for king-king proximity
    mopupEval += (14 - m_DistanceTable.Manhattan(friendlyKing, enemyKing)) * m_Constants.mopupProximityFactor;

    // Bonus for enemy king proximity to edge
    mopupEval += m_DistanceTable.ManhattanFromCenter(enemyKing) * m_Constants.mopupEdgeFactor;

    return mopupEval * (data.phase - LATEGAME_PHASE);
}

int32_t Evaluator::KingMobility(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

    int32_t kingMobilityEval = 0;

    if (data.phase > LATEGAME_PHASE)
        return 0;
    
    uint8_t kingIndex = data.kingIndices[state.turn];

    // Imagine a friendly queen where the king is and see how much it can move
    AttackMoveBuffer attacks;
    movegen::FindQueenAttacks(state, m_AttackTable, kingIndex, attacks);

    // Penalize excessive mobility (more than 3 squares)
    if (attacks.Size() > 3)
        kingMobilityEval += (attacks.Size() - 3) * m_Constants.kingMobilityFactor;

    return kingMobilityEval * (1.0 - data.phase);
}

int32_t Evaluator::Mobility(const BoardState& state) const 
{
    JUPITER_TRACE();

    std::size_t nAttacks = movegen::CountAllAttacks(state, m_AttackTable);
    std::size_t nQuiets = movegen::CountAllQuiets(state, m_AttackTable);
    return (nAttacks + nQuiets) * m_Constants.mobilityFactor;
}

int32_t Evaluator::KingPawnTropism(const BoardState& state, const PositionData& data) const 
{
    JUPITER_TRACE();

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

    Bitboard weak = data.pawns.weak[state.turn];
    Bitboard passed = data.pawns.passed[state.turn];

    SumDistances(state.pieces.Occupancy(state.turn, Piece::PAWN) & ~(weak | passed), PawnKind::REGULAR);
    SumDistances(weak, PawnKind::WEAK);
    SumDistances(passed, PawnKind::PASSED);

    // Avoid division by 0 if there are no pawns
    if (weightCount == 0)
        return 0;

    int32_t kpTropismEval = distances[PawnKind::REGULAR] * m_Constants.kingPawnTropismFactors[PawnKind::REGULAR]
        + distances[PawnKind::WEAK] * m_Constants.kingPawnTropismFactors[PawnKind::WEAK]
        + distances[PawnKind::PASSED] * m_Constants.kingPawnTropismFactors[PawnKind::PASSED];
    kpTropismEval /= weightCount;

    // More important in endgame, scale up
    return kpTropismEval * std::max(data.phase, LATEGAME_PHASE);
}

int32_t Evaluator::PawnShield(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    int32_t shieldEval = 0;

    if (data.phase > LATEGAME_PHASE)
        return 0;

    uint8_t kingFile = data.kingIndices[state.turn] & 7;
    uint8_t kingRank = data.kingIndices[state.turn] / 8;

    if ((state.turn == Color::WHITE && kingRank > 4) || (state.turn == Color::BLACK && kingRank < 3)) {
        // Shield can move up with the king
        uint8_t shieldRank = (2 * state.turn) + kingRank - 1;

        const PawnRanks& ranks = data.pawns.furthest[state.turn];

        // Only evaluate shield on the flanks
        uint8_t missingPawns = 0;
        if (kingFile < 3) {
            // Pawns on the 0, 1, 2 files need to be at shield rank
            if (ranks.Rank(0) != shieldRank) missingPawns++;
            if (ranks.Rank(1) != shieldRank) missingPawns++;
            if (ranks.Rank(2) != shieldRank) missingPawns++;
        } else if (kingFile > 4) {
            // Pawns on the 5, 6, 7 files need to be at shield rank
            if (ranks.Rank(5) != shieldRank) missingPawns++;
            if (ranks.Rank(6) != shieldRank) missingPawns++;
            if (ranks.Rank(7) != shieldRank) missingPawns++;
        }

        shieldEval += missingPawns * m_Constants.missingShieldPawnFactor;
    }

    return shieldEval * (1.0 - data.phase);}

int32_t Evaluator::PawnStorm(const BoardState& state, const PositionData& data) const
{
    JUPITER_TRACE();

    if (data.phase > LATEGAME_PHASE)
        return 0;

    // const PawnStructure::Relative& opp = data.pawns.relative[Color::Opposite(state.turn)];
    Color::Value enemy = Color::Opposite(state.turn);
    const PawnRanks& ranks = data.pawns.furthest[enemy];

    uint8_t kingIndex = data.kingIndices[state.turn];
    uint8_t kingFile = kingIndex & 7;
    
    int32_t stormEval = 0;

    // Count average distance to opponent pawns in current and adjacent files (further is better)
    uint8_t proximity = 23; // 8 + 7 + 8
    uint8_t rank0 = ranks.Rank(kingFile);
    uint8_t rank1 = (kingFile > 0) ? ranks.Rank(kingFile - 1) : UINT8_MAX;
    uint8_t rank2 = (kingFile < 7) ? ranks.Rank(kingFile + 1) : UINT8_MAX;

    if (rank0 < UINT8_MAX) proximity -= m_DistanceTable.Manhattan(kingIndex, ToIndex(kingFile, rank0));
    if (rank1 < UINT8_MAX) proximity -= m_DistanceTable.Manhattan(kingIndex, ToIndex(kingFile - 1, rank1));
    if (rank2 < UINT8_MAX) proximity -= m_DistanceTable.Manhattan(kingIndex, ToIndex(kingFile + 1, rank2));

    stormEval += proximity * m_Constants.stormingPawnFactor;

    return stormEval * (1.0 - data.phase);
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
    structureEval += std::popcount(data.pawns.weak[state.turn]) * m_Constants.weakPawnFactor;

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

// 0 - 24 (midgame - endgame)
void Evaluator::ComputeGamePhase(const BoardState& state, PositionData& data) const
{
    JUPITER_TRACE();

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    uint8_t nQueens = state.pieces.Count(friendly, Piece::QUEEN) + state.pieces.Count(enemy, Piece::QUEEN);
    uint8_t nRooks = state.pieces.Count(friendly, Piece::ROOK) + state.pieces.Count(enemy, Piece::ROOK);
    uint8_t nBishops = state.pieces.Count(friendly, Piece::BISHOP) + state.pieces.Count(enemy, Piece::BISHOP);
    uint8_t nKnights = state.pieces.Count(friendly, Piece::KNIGHT) + state.pieces.Count(enemy, Piece::KNIGHT);

    uint8_t phase = (QUEEN_PHASE * nQueens) + (ROOK_PHASE * nRooks) + (BISHOP_PHASE * nBishops) + (KNIGHT_PHASE * nKnights);
    phase = std::min(phase, MAX_PHASE); // Clamp in case of promotions

    data.phase = static_cast<float>(MAX_PHASE - phase) / MAX_PHASE;
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
    while (1) {
        Color::Value opponentTurn = Color::Opposite(turn);

        // Find attackers
        Bitboard attackers[Color::MAX_ENUM][Piece::MAX_ENUM];

        // Had to unroll the loop due to specialised functions being used per piece type
        attackers[turn][Piece::PAWN]   = m_AttackTable.PawnAttacks(move.to, opponentTurn) & pieces.Occupancy(turn, Piece::PAWN);
        attackers[turn][Piece::KNIGHT] = m_AttackTable.KnightAttacks(move.to) & pieces.Occupancy(turn, Piece::KNIGHT);
        attackers[turn][Piece::BISHOP] = m_AttackTable.BishopAttacks(move.to, pieces.Occupancy()) & pieces.Occupancy(turn, Piece::BISHOP);
        attackers[turn][Piece::ROOK]   = m_AttackTable.RookAttacks(move.to, pieces.Occupancy()) & pieces.Occupancy(turn, Piece::ROOK);
        attackers[turn][Piece::QUEEN]  = m_AttackTable.QueenAttacks(move.to, pieces.Occupancy()) & pieces.Occupancy(turn, Piece::QUEEN);
        attackers[turn][Piece::KING]   = m_AttackTable.KingAttacks(move.to) & pieces.Occupancy(turn, Piece::KING);

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

        // Early exit if the sign is decided
        if (gain.Size() > 1 && std::abs(gain[gain.Size() - 1]) >= Piece::Evaluate(Piece::QUEEN))
            break;

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

    const Bitboard FILE_MASK = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;

    // Open files
    {
        Bitboard blocked = 0;
        Bitboard pawns = state.pieces.Occupancy(Piece::PAWN);
        while (pawns) {
            uint8_t file = std::countr_zero(pawns) & 7;
            blocked |= (FILE_MASK << file);
            pawns &= (pawns - 1);
        }
        data.pawns.openFiles = ~blocked;
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
                data.pawns.weak[color] |= (1ull << index);
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
                data.pawns.weak[color] |= (1ull << index);

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
                data.pawns.furthest[color].Pack(file, index / 8);
                finishedFiles |= (1ull << file);
            }
            
            // For black we need to pop the MSB so we can't use the &= n - 1 trick
            furthestPawns = (color == Color::WHITE) ? furthestPawns & (furthestPawns - 1) : furthestPawns & ~(1ull << index);
        }

        // Passed pawns
        for (uint8_t file = 0; file < 8; file++) {
            // Passed pawns are strictly the frontmost in each rank
            uint8_t rank = data.pawns.furthest[color].Rank(file);
            if (rank == UINT8_MAX)
                continue;

            // No opponent pawns in the way means it is a passed pawn
            Bitboard opponentPawns = state.pieces.Occupancy(Color::Opposite(color), Piece::PAWN);
            if (!(opponentPawns & m_MaskTable.PassedPawnBlockers(color, 8 * rank + file)))
                data.pawns.passed[color] |= (1ull << (rank * 8 + file));
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
