#pragma once

#include "board/bitboard.h"
#include "board/zobrist.h"
#include "core.h"
#include "datastructure/buffer.h"
#include "evaluation/pieceSquareTable.h"

union LongAlgebraicMove {
    struct {
        char from[2];
        char to[2];
        char promote;
        char _padding[3];
    };
    char chars[8];

    bool IsValid() const { return chars[0] != '\0'; }
    static LongAlgebraicMove Invalid() { return LongAlgebraicMove { .chars = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' } }; }
    static LongAlgebraicMove FromChars(char *chars);
};

struct Move {
    uint8_t from{UINT8_MAX};
    uint8_t to{UINT8_MAX};
    Piece::Value piece{Piece::Invalid()};
    Piece::Value promote{Piece::Invalid()};

    LongAlgebraicMove ToLAN() const;
    bool operator==(const Move& other) const = default;

    bool IsValid() const { return Piece::IsValid(piece); }
    static Move Invalid() { return Move(); }
    static Move FromLAN(LongAlgebraicMove lan, const BitboardSet& piecePositions);
};

struct MoveData {
    ZobristKey zobristKey;
    Move move{Move::Invalid()};
    Piece::Value capture{Piece::Invalid()};
    CastlingRights rights{0};
    Color::Value turn{Color::Invalid()};
    uint8_t enPassantIndex{0};
    uint8_t fiftyMoveCounter{0};
    PSTScore pstScore{PSTScore(0, 0)};
};

const uint64_t MAX_POSSIBLE_QUIETS = 100;
const uint64_t MAX_POSSIBLE_ATTACKS = 100;

using QuietMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS>;
using AttackMoveBuffer = Buffer<Move, MAX_POSSIBLE_ATTACKS>;
using CombinedMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS + MAX_POSSIBLE_ATTACKS>;
