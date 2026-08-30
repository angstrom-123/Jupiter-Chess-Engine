#pragma once

#include <cstdint>
#include "core.h"

constexpr uint64_t ZOBRIST_NUMBER_COUNT = 781;

using ZobristKey = uint64_t;

class Zobrist {
public: 
    Zobrist();
    ZobristKey ComputeKey(const class BoardState& state) const; // Forward declare for recursive import fix
    uint64_t ValueForPiece(Color::Value color, Piece::Value piece, uint8_t index) const;
    uint64_t ValueForRights(CastlingRights rights) const;
    uint64_t ValueForEnPassant(uint8_t enPassantIndex) const;
    uint64_t ValueForTurn(Color::Value turn) const;

private:
    uint64_t m_Randoms[ZOBRIST_NUMBER_COUNT];
};
