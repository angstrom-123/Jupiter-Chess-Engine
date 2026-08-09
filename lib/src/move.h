#pragma once

#include "bitboard.h"
#include "core.h"

union LongAlgebraicMove {
    struct {
        char from[2];
        char to[2];
        char promote;
    };
    char chars[5];

    static LongAlgebraicMove Invalid() { return LongAlgebraicMove { .chars = { '\0', '\0', '\0', '\0', '\0' } }; }
    static bool IsValid(LongAlgebraicMove move) { return move.chars[0] != '\0'; }
    static LongAlgebraicMove FromChars(char *chars);
};

struct Move {
    uint8_t from{UINT8_MAX};
    uint8_t to{UINT8_MAX};
    Piece::Value piece{Piece::Invalid()};
    Piece::Value promote{Piece::Invalid()};

    LongAlgebraicMove ToLAN() const;
    bool operator==(const Move& other) const = default;

    static bool IsValid(Move move) { return Piece::IsValid(move.piece); }
    static Move Invalid() { return Move(); }
    static Move FromLAN(LongAlgebraicMove lan, const BitboardSet& piecePositions);
};
