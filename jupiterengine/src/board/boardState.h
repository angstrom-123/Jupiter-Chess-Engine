#pragma once

#include "board/bitboard.h"
#include "board/zobrist.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"

class BoardState {
public:
    BitboardSet pieces;
    ZobristKey zobristKey;
    CastlingRights rights{0};
    Color::Value turn{Color::WHITE};
    uint8_t enPassantIndex{UINT8_MAX};
    uint8_t fiftyMoveCounter{0};
    PSTScore pstScore{PSTScore(0, 0)};

    MoveData MakeMove(const Zobrist& zobrist, const PieceSquareTables& pst, Move move);
    void UnmakeMove(MoveData moveData);
    bool WasLegalMove(const class AttackTable& attackTable, MoveData moveData);
};
