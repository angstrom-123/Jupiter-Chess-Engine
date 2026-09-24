#pragma once

#include "datastructure/buffer.h"
#include <cstdint>
#include <cstdlib>

#ifdef _MSC_VER 
    #define INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define INLINE inline __attribute__((always_inline))
#else 
    #define INLINE inline
#endif

INLINE uint8_t ToIndex(uint8_t x, uint8_t y) { return x + 8 * y; }
INLINE uint8_t Difference(uint8_t a, uint8_t b) { return (a > b) ? a - b : b - a; }

struct Color {
    typedef enum : uint8_t {
        WHITE,
        BLACK,
        MAX_ENUM
    } Value;

    static constexpr Buffer<Color::Value, Color::MAX_ENUM> values{Color::WHITE, Color::BLACK};

    INLINE static Value Invalid() { return Value::MAX_ENUM; }
    INLINE static bool IsValid(Value color) { return color < Value::MAX_ENUM; }
    INLINE static Value Opposite(Value color) 
    { 
        return static_cast<Value>(1 - color);
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

    static constexpr Buffer<Piece::Value, Piece::MAX_ENUM> values{Piece::PAWN, Piece::KNIGHT, Piece::BISHOP, Piece::ROOK, Piece::QUEEN, Piece::KING};

    INLINE static Value Invalid() { return Value::MAX_ENUM; }
    INLINE static bool IsValid(Value value) { return value < Value::MAX_ENUM; }
    static const char *Show(Value value) { return m_Names[value]; }
    INLINE static int32_t Evaluate(Value value) { return m_Evals[value]; }

private:
    static constexpr int32_t m_Evals[Piece::MAX_ENUM] = { 100, 300, 310, 500, 975, 0 };
    static constexpr const char *m_Names[Piece::MAX_ENUM + 1] = { "Pawn", "Knight", "Bishop", "Rook", "Queen", "King", "None" };
};
INLINE Piece::Value operator++(Piece::Value& value, int)
{
    Piece::Value original = value;
    value = static_cast<Piece::Value>(static_cast<uint8_t>(value) + 1);
    return original;
}

using CastlingRights = uint8_t;
struct CastlingRight {
    typedef enum : uint8_t {
        KINGSIDE_WHITE = 0x1,
        KINGSIDE_BLACK = 0x2,
        QUEENSIDE_WHITE = 0x4,
        QUEENSIDE_BLACK = 0x8,
    } Value;
    INLINE static consteval CastlingRights All() { return KINGSIDE_WHITE | KINGSIDE_BLACK | QUEENSIDE_WHITE | QUEENSIDE_BLACK; }
    INLINE static Value Kingside(Color::Value color)
    {
        return static_cast<Value>(1 + color);
    }
    INLINE static Value Queenside(Color::Value color)
    {
        return static_cast<Value>(4 + 4 * color);
    }
};

struct FenView {
    uint64_t start;
    uint64_t end;
};
