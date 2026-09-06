#include "movegen.h"
#include "board/bitboard.h"
#include "core.h"
#include "movegen/move.h"
#include "util/instrumenter.h"
#include <bit>

void Movegen::FindAllAttacks(AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    Bitboard queens = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
    while (queens) {
        uint8_t index = std::countr_zero(queens);
        FindQueenAttacks(index, attacks);
        queens &= (queens - 1);
    }


    Bitboard rooks = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
    while (rooks) {
        uint8_t index = std::countr_zero(rooks);
        FindRookAttacks(index, attacks);
        rooks &= (rooks - 1);
    }

    Bitboard bishops = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
    while (bishops) {
        uint8_t index = std::countr_zero(bishops);
        FindBishopAttacks(index, attacks);
        bishops &= (bishops - 1);
    }

    Bitboard knights = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
    while (knights) {
        uint8_t index = std::countr_zero(knights);
        FindKnightAttacks(index, attacks);
        knights &= (knights - 1);
    }

    FindAllPawnAttacks(m_State.turn, attacks);

    Bitboard kings = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
    while (kings) {
        uint8_t index = std::countr_zero(kings);
        FindKingAttacks(index, attacks);
        kings &= (kings - 1);
    }
}

void Movegen::FindAllQuiets(QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    Bitboard queens = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
    while (queens) {
        uint8_t index = std::countr_zero(queens);
        FindQueenQuiets(index, quiets);
        queens &= (queens - 1);
    }

    Bitboard rooks = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
    while (rooks) {
        uint8_t index = std::countr_zero(rooks);
        FindRookQuiets(index, quiets);
        rooks &= (rooks - 1);
    }

    Bitboard bishops = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
    while (bishops) {
        uint8_t index = std::countr_zero(bishops);
        FindBishopQuiets(index, quiets);
        bishops &= (bishops - 1);
    }

    Bitboard knights = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
    while (knights) {
        uint8_t index = std::countr_zero(knights);
        FindKnightQuiets(index, quiets);
        knights &= (knights - 1);
    }

    FindAllPawnQuiets(m_State.turn, quiets);

    Bitboard kings = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
    while (kings) {
        uint8_t index = std::countr_zero(kings);
        FindKingQuiets(index, quiets);
        kings &= (kings - 1);
    }
}

void Movegen::FindAllPawnAttacks(Color::Value color, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();
    Pawngen::attackFunctions[color](std::forward<const BoardState>(m_State), attacks);
}

void Movegen::FindAllPawnQuiets(Color::Value color, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();
    Pawngen::quietFunctions[color](std::forward<const BoardState>(m_State), quiets);
}

void Movegen::FindKnightAttacks(uint8_t index, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetKnightAttacks(index);
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        attacks.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindKnightQuiets(uint8_t index, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetKnightAttacks(index);
    attackBits &= ~m_State.pieces.OccupancyMask();

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        quiets.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindKingAttacks(uint8_t index, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetKingAttacks(index);
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        attacks.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindKingQuiets(uint8_t index, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    // Attacks
    {
        Bitboard attackBits = m_AttackTable.GetKingAttacks(index);
        attackBits &= ~m_State.pieces.OccupancyMask();

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            quiets.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }

    // Castling
    {
        if ((m_State.rights & CastlingRight::Kingside(m_State.turn)) && !m_State.pieces.HasAny({ index + 1ull, index + 2ull }))
            quiets.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

        if ((m_State.rights & CastlingRight::Queenside(m_State.turn)) && !m_State.pieces.HasAny({ index - 1ull, index - 2ull, index - 3ull }))
            quiets.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
    }
}

void Movegen::FindBishopAttacks(uint8_t index, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetBishopAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    FindSliderAttacks(index, Piece::BISHOP, attackBits, attacks);
}

void Movegen::FindBishopQuiets(uint8_t index, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetBishopAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();
    FindSliderQuiets(index, Piece::BISHOP, attackBits, quiets);
}

void Movegen::FindRookAttacks(uint8_t index, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetRookAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    FindSliderAttacks(index, Piece::ROOK, attackBits, attacks);
}

void Movegen::FindRookQuiets(uint8_t index, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetRookAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();
    FindSliderQuiets(index, Piece::ROOK, attackBits, quiets);
}

void Movegen::FindQueenAttacks(uint8_t index, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetQueenAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    FindSliderAttacks(index, Piece::QUEEN, attackBits, attacks);
}

void Movegen::FindQueenQuiets(uint8_t index, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetQueenAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();
    FindSliderQuiets(index, Piece::QUEEN, attackBits, quiets);
}

void Movegen::FindSliderAttacks(uint8_t index, Piece::Value piece, Bitboard attackBits, AttackMoveBuffer& attacks) const
{
    JUPITER_TRACE();

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        attacks.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

void Movegen::FindSliderQuiets(uint8_t index, Piece::Value piece, Bitboard attackBits, QuietMoveBuffer& quiets) const
{
    JUPITER_TRACE();

    while (attackBits) {
        uint8_t toIndex = std::countr_zero(attackBits);
        quiets.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attackBits &= (attackBits - 1);
    }
}

std::size_t Movegen::CountAllAttacks() const
{
    JUPITER_TRACE();

    std::size_t total = 0;

    Bitboard queens = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
    while (queens) {
        uint8_t index = std::countr_zero(queens);
        total += CountQueenAttacks(index);
        queens &= (queens - 1);
    }


    Bitboard rooks = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
    while (rooks) {
        uint8_t index = std::countr_zero(rooks);
        total += CountRookAttacks(index);
        rooks &= (rooks - 1);
    }

    Bitboard bishops = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
    while (bishops) {
        uint8_t index = std::countr_zero(bishops);
        total += CountBishopAttacks(index);
        bishops &= (bishops - 1);
    }

    Bitboard knights = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
    while (knights) {
        uint8_t index = std::countr_zero(knights);
        total += CountKnightAttacks(index);
        knights &= (knights - 1);
    }

    total += CountAllPawnAttacks(m_State.turn);

    Bitboard kings = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
    while (kings) {
        uint8_t index = std::countr_zero(kings);
        total += CountKingAttacks(index);
        kings &= (kings - 1);
    }

    return total;
}

std::size_t Movegen::CountAllQuiets() const
{
    JUPITER_TRACE();

    std::size_t total = 0;

    Bitboard queens = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
    while (queens) {
        uint8_t index = std::countr_zero(queens);
        total += CountQueenQuiets(index);
        queens &= (queens - 1);
    }


    Bitboard rooks = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
    while (rooks) {
        uint8_t index = std::countr_zero(rooks);
        total += CountRookQuiets(index);
        rooks &= (rooks - 1);
    }

    Bitboard bishops = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
    while (bishops) {
        uint8_t index = std::countr_zero(bishops);
        total += CountBishopQuiets(index);
        bishops &= (bishops - 1);
    }

    Bitboard knights = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
    while (knights) {
        uint8_t index = std::countr_zero(knights);
        total += CountKnightQuiets(index);
        knights &= (knights - 1);
    }

    total += CountAllPawnQuiets(m_State.turn);

    Bitboard kings = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
    while (kings) {
        uint8_t index = std::countr_zero(kings);
        total += CountKingQuiets(index);
        kings &= (kings - 1);
    }

    return total;
}

std::size_t Movegen::CountAllPawnAttacks(Color::Value color) const 
{
    return Pawngen::attackCountFunctions[color](std::forward<const BoardState>(m_State));
}

std::size_t Movegen::CountAllPawnQuiets(Color::Value color) const 
{
    return Pawngen::quietCountFunctions[color](std::forward<const BoardState>(m_State));
}

std::size_t Movegen::CountKnightAttacks(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetKnightAttacks(index);
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    return std::popcount(attackBits);
}

std::size_t Movegen::CountKnightQuiets(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetKnightAttacks(index);
    attackBits &= ~m_State.pieces.OccupancyMask();
    return std::popcount(attackBits);
}

std::size_t Movegen::CountBishopAttacks(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetBishopAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    return std::popcount(attackBits);
}

std::size_t Movegen::CountBishopQuiets(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetBishopAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();
    return std::popcount(attackBits);
}

std::size_t Movegen::CountRookAttacks(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetRookAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    return std::popcount(attackBits);
}

std::size_t Movegen::CountRookQuiets(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetRookAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();
    return std::popcount(attackBits);
}

std::size_t Movegen::CountQueenAttacks(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetQueenAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    return std::popcount(attackBits);
}

std::size_t Movegen::CountQueenQuiets(uint8_t index) const 
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetQueenAttacks(index, m_State.pieces.OccupancyMask());
    attackBits &= ~m_State.pieces.OccupancyMask();
    return std::popcount(attackBits);
}

std::size_t Movegen::CountKingAttacks(uint8_t index) const
{
    JUPITER_TRACE();

    Bitboard attackBits = m_AttackTable.GetKingAttacks(index);
    attackBits &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));
    return std::popcount(attackBits);
}

std::size_t Movegen::CountKingQuiets(uint8_t index) const
{
    JUPITER_TRACE();

    std::size_t total = 0;

    Bitboard attackBits = m_AttackTable.GetKingAttacks(index);
    attackBits &= ~m_State.pieces.OccupancyMask();
    total += std::popcount(attackBits);

    if ((m_State.rights & CastlingRight::Kingside(m_State.turn)) && !m_State.pieces.HasAny({ index + 1ull, index + 2ull }))
        total++;

    if ((m_State.rights & CastlingRight::Queenside(m_State.turn)) && !m_State.pieces.HasAny({ index - 1ull, index - 2ull, index - 3ull }))
        total++;

    return total;
}
