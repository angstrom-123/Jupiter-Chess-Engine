#pragma once

#include "bitboard.h"
#include "zobrist.h"

class BoardState {
public:
    BitboardSet pieces;
    ZobristKey zobristKey;
    CastlingRights rights{0};
    Color::Value turn{Color::WHITE};
    uint8_t enPassantIndex{UINT8_MAX};
    uint16_t halfMoves{0};
    uint8_t fiftyMoveCounter{0};
};
