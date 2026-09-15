#pragma once

#include "core.h"
#include <utility>

using Bitboard = uint64_t;

const Bitboard FULL_BITBOARD = UINT64_MAX;

inline void ShowBitboard(Bitboard bb) 
{
    std::stringstream ss;
    for (std::size_t i = 0; i < 64; i++) {
        if (i % 8 == 0)
            ss << std::endl;

        ss << ((bb & (1ull << i)) ? "x " : ". ");
    }
    ss << std::endl;
    INFO(ss.str());
}

class BitboardSet {
public:
    BitboardSet();
    INLINE void Set(Color::Value color, Piece::Value piece, uint8_t index)
    {
        uint64_t bit = 1ull << index;
        m_Bits[color][piece] |= bit;
        m_Combined[color] |= bit;
    }
    INLINE void Unset(Color::Value color, Piece::Value piece, uint8_t index)
    {
        uint64_t bit = 1ull << index;
        m_Bits[color][piece] &= ~bit;
        m_Combined[color] &= ~bit;
    }
    INLINE void UnsetAll(Color::Value color, uint8_t index)
    {
        uint64_t bit = 1ull << index;
        if (m_Combined[color] & bit) {
            m_Bits[color][Piece::PAWN] &= ~bit;
            m_Bits[color][Piece::KNIGHT] &= ~bit;
            m_Bits[color][Piece::BISHOP] &= ~bit;
            m_Bits[color][Piece::ROOK] &= ~bit;
            m_Bits[color][Piece::QUEEN] &= ~bit;
            m_Bits[color][Piece::KING] &= ~bit;
            m_Combined[color] &= ~bit;
        }
    }
    INLINE bool Has(Color::Value color, Piece::Value piece, uint8_t index) const { return m_Bits[color][piece] & (1ull << index); }
    INLINE bool Has(Color::Value color, uint8_t index) const { return m_Combined[color] & (1ull << index); }
    INLINE bool Has(uint8_t index) const { return (m_Combined[Color::WHITE] | m_Combined[Color::BLACK]) & (1ull << index); }
    INLINE uint8_t Count(Color::Value color, Piece::Value piece) const { return std::popcount(m_Bits[color][piece]); }
    INLINE Bitboard Occupancy(Piece::Value piece) const { return m_Bits[Color::WHITE][piece] | m_Bits[Color::BLACK][piece]; }
    INLINE Bitboard Occupancy(Color::Value color, Piece::Value piece) const { return m_Bits[color][piece]; }
    INLINE Bitboard Occupancy(Color::Value color) const { return m_Combined[color]; }
    INLINE Bitboard Occupancy() const { return m_Combined[Color::WHITE] | m_Combined[Color::BLACK]; }

    std::pair<Color::Value, Piece::Value> PieceInSquare(uint8_t index) const;
    Piece::Value PieceInSquare(Color::Value color, uint8_t index) const;
    void Clear();
    void Show() const;
    void Dump() const;
    void Validate() const;

private:
    uint64_t m_Bits[Color::MAX_ENUM][Piece::MAX_ENUM]{{0}};
    uint64_t m_Combined[Color::MAX_ENUM]{0};
};
