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

    INLINE bool IsValid() const { return chars[0] != '\0'; }
    INLINE static LongAlgebraicMove Invalid() { return LongAlgebraicMove { .chars = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' } }; }
    static LongAlgebraicMove FromChars(char *chars);
};

union Move {
    struct {
        uint8_t from{UINT8_MAX};
        uint8_t to{UINT8_MAX};
        Piece::Value piece{Piece::Invalid()};
        Piece::Value promote{Piece::Invalid()};
    };
    uint32_t bits; // For comparisons, Pack() gives a smaller (18 bit) representation 

    Move() = default;
    constexpr Move(uint8_t from, uint8_t to, Piece::Value piece, Piece::Value promote = Piece::Invalid())
        : from{from}, to{to}, piece{piece}, promote{promote} {}

    LongAlgebraicMove ToLAN() const;
    INLINE bool operator==(const Move& other) const { return bits == other.bits; }
    INLINE uint32_t Pack() const 
    {
        uint32_t packed = 0;
        packed |= static_cast<uint32_t>(promote & 0b111) << 15;
        packed |= static_cast<uint32_t>(piece & 0b111) << 12;
        packed |= static_cast<uint32_t>(to & 0b111111) << 6;
        packed |= static_cast<uint32_t>(from & 0b111111);
        return packed;
    }

    INLINE bool IsValid() const 
    { 
        return Piece::IsValid(piece) 
            && from < 64 
            && to < 64 
            && promote != Piece::PAWN 
            && (promote == Piece::Invalid() || piece == Piece::PAWN); 
    }
    INLINE static constexpr Move Invalid() { return Move{ UINT8_MAX, UINT8_MAX, Piece::Invalid(), Piece::Invalid() }; }
    INLINE static Move Unpack(uint32_t packed)
    {
        return Move(
            static_cast<uint8_t>(packed & 0b111111),            // from
            static_cast<uint8_t>((packed >> 6) & 0b111111),     // to
            static_cast<Piece::Value>((packed >> 12) & 0b111),  // piece
            static_cast<Piece::Value>((packed >> 15) & 0b111)   // promote
        );
    }
    INLINE static uint8_t UnpackFrom(uint32_t packed) { return static_cast<uint8_t>(packed & 0b111111); }
    INLINE static uint8_t UnpackTo(uint32_t packed) { return static_cast<uint8_t>((packed >> 6) & 0b111111); }
    static Move FromLAN(LongAlgebraicMove lan, const BitboardSet& piecePositions);
    void Show()
    {
        std::stringstream ss;
        ss << "from: " << (int) from << ", " << std::endl;
        ss << "to: " << (int) to << ", " << std::endl;
        ss << "piece: " << Piece::Show(piece) << "," << std::endl;
        ss << "promote: " << Piece::Show(promote) << "," << std::endl;
        INFO(ss.str());
    }
};

struct MoveData {
    ZobristKey zobristKey;
    ZobristKey pawnKey;
    Move move{Move::Invalid()};
    Piece::Value capture{Piece::Invalid()};
    CastlingRights rights{0};
    Color::Value turn{Color::Invalid()};
    uint8_t enPassantIndex{0};
    uint8_t fiftyMoveCounter{0};
    PSTScore pstScore{PSTScore(0, 0)};
};

// TODO: Don't forget about these limits! (17/11/2026)
//       My program does not currently do ANY bounds checks and will segfault in rare positions.
//       This is NOT a safe assumption, but is extremely unlikely to bite me in regular play.
//       If someone starts messing with the starting position, this WILL BREAK.
const uint64_t MAX_QUIETS = 96; // Theoretical upper bound is 218 (9 queens)
const uint64_t MAX_ATTACKS = 32; // Theoretical upper bound is 74 (9 queens)

using QuietMoveBuffer = Buffer<Move, MAX_QUIETS>;
using AttackMoveBuffer = Buffer<Move, MAX_ATTACKS>;
using PackedQuietMoveBuffer = Buffer<uint32_t, MAX_QUIETS>;
using PackedAttackMoveBuffer = Buffer<uint32_t, MAX_ATTACKS>;
using CombinedMoveBuffer = Buffer<Move, MAX_QUIETS + MAX_ATTACKS>;
