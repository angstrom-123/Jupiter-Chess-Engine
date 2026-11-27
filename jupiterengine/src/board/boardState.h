#pragma once

#include "board/bitboard.h"
#include "board/zobrist.h"
#include "evaluation/pieceSquareTable.h"
#include "movegen/move.h"

class BoardState {
public:
    BoardState() = default;
    BoardState(const BoardState& other) = default;
    BoardState& operator=(const BoardState& other) noexcept;
    MoveData MakeMove(const Zobrist& zobrist, const PieceSquareTables& pst, Move move);
    void MakeMoveNoIncremental(Move move);
    void UnmakeMove(MoveData moveData);
    bool WasLegalMove(const class AttackTable& attackTable, MoveData moveData) const;
    bool IsPseudoLegal(const AttackMoveBuffer& attacks, const QuietMoveBuffer& quiets, Move move) const;
    bool KingUnderAttack(const AttackTable& attackTable, Color::Value turn) const;
    bool KingUnderAttack(const AttackTable& attackTable) const;

public:
    BitboardSet pieces;
    ZobristKey zobristKey;
    ZobristKey pawnKey;
    PSTScore pstScore{PSTScore(0, 0)};
    CastlingRights rights{0};
    Color::Value turn{Color::WHITE};
    uint8_t enPassantIndex{UINT8_MAX};
    uint8_t fiftyMoveCounter{0};
    uint16_t halfMove{0};
};
