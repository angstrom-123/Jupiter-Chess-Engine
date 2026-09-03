#pragma once

#include "util/exception.h"
#include <cstdint>
#include <cstdlib>
#include <string>

uint8_t ToIndex(uint8_t x, uint8_t y);
uint8_t Difference(uint8_t a, uint8_t b);

struct Color {
    typedef enum : uint8_t {
        WHITE,
        BLACK,
        MAX_ENUM
    } Value;

    static Value Invalid() { return Value::MAX_ENUM; }
    static bool IsValid(Value value) { return value < Value::MAX_ENUM; }
    static Value Opposite(Value value) 
    { 
        switch (value) {
            case Value::WHITE: return Value::BLACK;
            case Value::BLACK: return Value::WHITE;
            default: throw JupiterException(std::string("Getting opposite of invalid colour: ") + Color::Show(value));
        }
    }
    static const char *Show(Value value)
    {
        switch (value) {
            case WHITE: return "White";
            case BLACK: return "Black";
            default: return "None";
        }
    }
};

struct Piece {
    typedef enum : uint8_t {
        PAWN,
        KNIGHT,
        BISHOP,
        ROOK,
        QUEEN,
        KING,
        MAX_ENUM
    } Value;

    static Value Invalid() { return Value::MAX_ENUM; }
    static bool IsValid(Value value) { return value < Value::MAX_ENUM; }
    static const char *Show(Value value)
    {
        switch (value) {
            case PAWN: return "Pawn";
            case KNIGHT: return "Knight";
            case BISHOP: return "Bishop";
            case ROOK: return "Rook";
            case QUEEN: return "Queen";
            case KING: return "King";
            default: return "None";
        }
    }
    static int64_t Evaluate(Value value) 
    {
        switch (value) {
            case PAWN: return 100;
            case KNIGHT: return 300;
            case BISHOP: return 310;
            case ROOK: return 500;
            case QUEEN: return 975;
            case KING: return 0;
            default: throw JupiterException(std::string("Evaluating invalid piece: ") + Piece::Show(value));
        }
    }
};

using CastlingRights = uint8_t;
struct CastlingRight {
    typedef enum : uint8_t {
        KINGSIDE_WHITE = 0x1,
        KINGSIDE_BLACK = 0x2,
        QUEENSIDE_WHITE = 0x4,
        QUEENSIDE_BLACK = 0x8,
    } Value;
    static Value Kingside(Color::Value color)
    {
        switch (color) {
            case Color::WHITE: return KINGSIDE_WHITE;
            case Color::BLACK: return KINGSIDE_BLACK;
            default: throw JupiterException(std::string("Getting kingside castling right for invalid colour: ") + Color::Show(color));
        }
    }
    static Value Queenside(Color::Value color)
    {
        switch (color) {
            case Color::WHITE: return QUEENSIDE_WHITE;
            case Color::BLACK: return QUEENSIDE_BLACK;
            default: throw JupiterException(std::string("Getting queenside castling right for invalid colour: ") + Color::Show(color));
        }
    }
};

struct FenView {
    uint64_t start;
    uint64_t end;
};
