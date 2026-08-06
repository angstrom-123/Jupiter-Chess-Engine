#include "move.h"
#include <cmath>
#include <cstring>

LongAlgebraicMove LongAlgebraicMove::FromChars(char *chars)
{
    uint64_t len = std::strlen(chars);
    if (len < 4 || len > 5)
        return LongAlgebraicMove::Invalid();

    if (chars[0] < 'a' || chars[0] > 'h' || 
            chars[1] < '1' || chars[1] > '8' ||
            chars[2] < 'a' || chars[2] > 'h' || 
            chars[3] < '1' || chars[3] > '8')
        return LongAlgebraicMove::Invalid();

    LongAlgebraicMove move;
    move.from[0] = chars[0];
    move.from[1] = chars[1];
    move.to[0] = chars[2];
    move.to[1] = chars[3];

    if (len == 5) {
        if (chars[4] != 'n' && chars[4] != 'b' && chars[4] != 'r' && chars[4] != 'q')
            return LongAlgebraicMove::Invalid();

        move.promote = chars[4];
    }
    return move;
}

LongAlgebraicMove Move::ToLAN() const
{
    auto ToAlgebraic = [](uint8_t index, char *result) {
        uint8_t x = index % 8;
        uint8_t y = index / 8;
        result[0] = 'a' + x;
        result[1] = '8' - y;
    };

    LongAlgebraicMove lan =  { .chars = { '\0', '\0', '\0', '\0', '\0' } };

    ToAlgebraic(from, lan.from);
    ToAlgebraic(to, lan.to);

    if (Piece::IsValid(promote)) {
        switch (promote) {
            case Piece::KNIGHT:
                lan.promote = 'n';
                break;
            case Piece::BISHOP:
                lan.promote = 'b';
                break;
            case Piece::ROOK:
                lan.promote = 'r';
                break;
            case Piece::QUEEN:
                lan.promote = 'q';
                break;
            default:
                return LongAlgebraicMove::Invalid();
        }
    }

    return lan;
}

Move Move::FromLAN(LongAlgebraicMove lan, const BitboardSet& piecePositions)
{
    Move move;
    auto ToIndex = [](const char *algebraic) {
        uint8_t x = algebraic[0] - 'a';
        uint8_t y = '8' - algebraic[1];
        return x + 8 * y;
    };
    move.from = ToIndex(lan.from);
    move.to = ToIndex(lan.to);
    move.piece = piecePositions.PieceInSquare(move.from).second;
    switch (lan.promote) {
        case 'n':
            move.promote = Piece::KNIGHT;
            break;
        case 'b':
            move.promote = Piece::BISHOP;
            break;
        case 'r':
            move.promote = Piece::ROOK;
            break;
        case 'q':
            move.promote = Piece::QUEEN;
            break;
        default:
            move.promote = Piece::Invalid();
            break;
    }
    return move;
}
