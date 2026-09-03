#include "movegen.h"
#include "board/bitboard.h"
#include "core.h"
#include "util/instrumenter.h"
#include <bit>

void Movegen::FindAllAttacks(AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    // Queens
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindQueenAttacks(index, attacks);
            occupancy &= (occupancy - 1);
        }
    }

    // Bishops
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindBishopAttacks(index, attacks);
            occupancy &= (occupancy - 1);
        }
    }

    // Pawns
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::PAWN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindPawnAttacks(index, attacks);
            occupancy &= (occupancy - 1);
        }
    }

    // Knights
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKnightAttacks(index, attacks);
            occupancy &= (occupancy - 1);
        }
    }

    // Rooks
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindRookAttacks(index, attacks);
            occupancy &= (occupancy - 1);
        }
    }

    // Kings
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKingAttacks(index, attacks);
            occupancy &= (occupancy - 1);
        }
    }
}

void Movegen::FindAllQuiets(QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    // Pawns
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::PAWN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindPawnQuiets(index, quiets);
            occupancy &= (occupancy - 1);
        }
    }

    // Knights
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKnightQuiets(index, quiets);
            occupancy &= (occupancy - 1);
        }
    }

    // Bishops
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindBishopQuiets(index, quiets);
            occupancy &= (occupancy - 1);
        }
    }

    // Rooks
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindRookQuiets(index, quiets);
            occupancy &= (occupancy - 1);
        }
    }

    // Queens
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindQueenQuiets(index, quiets);
            occupancy &= (occupancy - 1);
        }
    }

    // Kings
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKingQuiets(index, quiets);
            occupancy &= (occupancy - 1);
        }
    }
}

void Movegen::FindPawnAttacks(uint8_t index, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back

    uint64_t enPassantBit = (m_State.enPassantIndex != UINT8_MAX) ? 1ul << m_State.enPassantIndex : 0ul;
    Bitboard attackBits = m_AttackTable.GetAttacks(index, Piece::PAWN, m_State.turn, m_State.pieces.OccupancyMask());
    attackBits &= (m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn)) | enPassantBit);
    
    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        uint64_t toBit = 1ul << toIndex;
        if (toBit & backRankMask) {
            // Promotion
            attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
            attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
            attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
            attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
        } else {
            attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
        }
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindPawnQuiets(uint8_t index, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back
    const uint64_t homeSquareMask = (m_State.turn == Color::WHITE) ? 0xFF000000000000 : 0x000000000000FF00;

    // In all cases, need to check if the move is a promotion

    int8_t delta = (m_State.turn == Color::WHITE) ? -8 : 8;
    uint8_t toIndex = index + delta;
    uint64_t toBit = 1ul << toIndex;
    if (!m_State.pieces.Has(toIndex)) {
        if (toBit & backRankMask) {
            // Promotion
            quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
            quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
            quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
            quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
        } else {
            quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            toIndex += delta;
            if (((1ul << index) & homeSquareMask) && !m_State.pieces.Has(toIndex))
                quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
        }
    }
}

void Movegen::FindKnightAttacks(uint8_t index, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetAttacks(index, Piece::KNIGHT, m_State.turn, 0);
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        attacks.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindKnightQuiets(uint8_t index, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetAttacks(index, Piece::KNIGHT, m_State.turn, 0);
    attackBits &= ~m_State.pieces.OccupancyMask();

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        quiets.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindKingAttacks(uint8_t index, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetAttacks(index, Piece::KING, m_State.turn, 0);
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        attacks.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindKingQuiets(uint8_t index, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    // Attacks
    {
        Bitboard attackBits = m_AttackTable.GetAttacks(index, Piece::KING, m_State.turn, 0);
        attackBits &= ~m_State.pieces.OccupancyMask();

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            quiets.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }

    // Castling
    {
        if ((m_State.rights & CastlingRight::Kingside(m_State.turn)) && !m_State.pieces.HasAny({ index + 1ul, index + 2ul }))
            quiets.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

        if ((m_State.rights & CastlingRight::Queenside(m_State.turn)) && !m_State.pieces.HasAny({ index - 1ul, index - 2ul, index - 3ul }))
            quiets.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
    }
}

void Movegen::FindBishopAttacks(uint8_t index, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    FindSliderAttacks(index, Piece::BISHOP, attacks);
}

void Movegen::FindBishopQuiets(uint8_t index, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    FindSliderQuiets(index, Piece::BISHOP, quiets);
}

void Movegen::FindRookAttacks(uint8_t index, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    FindSliderAttacks(index, Piece::ROOK, attacks);
}

void Movegen::FindRookQuiets(uint8_t index, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    FindSliderQuiets(index, Piece::ROOK, quiets);
}

void Movegen::FindQueenAttacks(uint8_t index, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    FindSliderAttacks(index, Piece::QUEEN, attacks);
}

void Movegen::FindQueenQuiets(uint8_t index, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    FindSliderQuiets(index, Piece::QUEEN, quiets);
}

void Movegen::FindSliderAttacks(uint8_t index, Piece::Value piece, AttackMoveBuffer& attacks)
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetAttacks(index, piece, m_State.turn, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        attacks.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindSliderQuiets(uint8_t index, Piece::Value piece, QuietMoveBuffer& quiets)
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetAttacks(index, piece, m_State.turn, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        quiets.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}
