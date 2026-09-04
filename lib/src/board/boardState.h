#pragma once

#include "board/bitboard.h"
#include "board/zobrist.h"
#include "evaluation/pieceSquareTable.h"

class BoardState {
public:
    BitboardSet pieces;
    ZobristKey zobristKey;
    CastlingRights rights{0};
    Color::Value turn{Color::WHITE};
    uint8_t enPassantIndex{UINT8_MAX};
    uint8_t fiftyMoveCounter{0};
    PSTScore pstScore{PSTScore(0, 0)};
};
