#pragma once

#include "bitboard.h"

class BoardState {
public:
    BitboardSet pieces;
    CastlingRights rights{0};
    Color::Value turn{Color::WHITE};
    uint8_t enPassantIndex{UINT8_MAX};
    uint64_t halfMoves{0};
};
