#pragma once

#include <cstdint>
#include "core.h"

constexpr uint64_t N_ZOBRIST_NUMBERS = 781;

using ZobristKey = uint64_t;

class Zobrist {
public: 
    Zobrist();
    ZobristKey ComputeKey(const class BoardState& state) const; // Forward declare for recursive import fix
    ZobristKey ComputePawnKey(const class BoardState& state) const;
    INLINE uint64_t ValueForPiece(Color::Value color, Piece::Value piece, uint8_t index) const
    {
        uint64_t colorOffset = color * (Piece::MAX_ENUM * 64);
        uint64_t pieceOffset = piece * 64;
        uint64_t indexOffset = index;
        return m_Randoms[colorOffset + pieceOffset + indexOffset];
    }
    INLINE uint64_t ValueForRights(CastlingRights rights) const
    {
        constexpr uint64_t rightsOffset = 2 * (Piece::MAX_ENUM * 64);
        uint64_t value = 0;
        if (rights & CastlingRight::KINGSIDE_WHITE)
            value ^= m_Randoms[rightsOffset];
        if (rights & CastlingRight::QUEENSIDE_WHITE)
            value ^= m_Randoms[rightsOffset + 1];
        if (rights & CastlingRight::KINGSIDE_BLACK)
            value ^= m_Randoms[rightsOffset + 2];
        if (rights & CastlingRight::QUEENSIDE_BLACK)
            value ^= m_Randoms[rightsOffset + 3];
        return value;
    }
    INLINE uint64_t ValueForEnPassant(uint8_t enPassantIndex) const
    {
        constexpr uint64_t enPassantOffset = 2 * (Piece::MAX_ENUM * 64) + 4;

        if (enPassantIndex == UINT8_MAX)
            return 0;

        uint8_t file = enPassantIndex & 7;
        return m_Randoms[enPassantOffset + file];
    }
    INLINE uint64_t ValueForTurn() const
    {
        return m_Randoms[N_ZOBRIST_NUMBERS - 1];
    }

private:
    uint64_t m_Randoms[N_ZOBRIST_NUMBERS];
};
