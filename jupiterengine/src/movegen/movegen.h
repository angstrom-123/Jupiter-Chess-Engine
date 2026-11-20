#pragma once

#include <cassert>
#include <cstdint>
#include "board/zobrist.h"
#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "core.h"
#include "util/instrumenter.h"

using PawnAttackFunction = void (*)(const BoardState&, AttackMoveBuffer&);
using PawnQuietFunction = void (*)(const BoardState&, QuietMoveBuffer&);
using PawnCountFunction = std::size_t (*)(const BoardState& state);

// Pawns move funny so I generate their moves using bitshifts so they can all be done at once.
// This is templated for moving color to avoid runtime checks.
namespace pawngen {
    template<Color::Value color> void FindPawnAttacks(const BoardState& state, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard A_FILE_MASK = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;
        Bitboard H_FILE_MASK = 0b10000000'10000000'10000000'10000000'10000000'10000000'10000000'10000000;

        uint64_t enPassantBit = (state.enPassantIndex != UINT8_MAX) ? 1ull << state.enPassantIndex : 0ull;

        Bitboard captureLeft;
        Bitboard captureRight;
        constexpr int8_t shiftLeft = (color == Color::WHITE) ? 9 : -7;
        constexpr int8_t shiftRight = (color == Color::WHITE) ? 7 : -9;
        if constexpr (color == Color::WHITE) {
            Bitboard targetBits = state.pieces.Occupancy(Color::BLACK) | enPassantBit;
            Bitboard pawns = state.pieces.Occupancy(Color::WHITE, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) >> 9) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) >> 7)  & targetBits;
        } else {
            Bitboard targetBits = state.pieces.Occupancy(Color::WHITE) | enPassantBit;
            Bitboard pawns = state.pieces.Occupancy(Color::BLACK, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) << 7) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) << 9)  & targetBits;
        }

        Bitboard normalCaptureLeft = captureLeft & ~BACK_RANK_MASK;
        while (normalCaptureLeft) {
            uint8_t index = std::countr_zero(normalCaptureLeft);
            uint8_t from = index + shiftLeft;
            attacks.EmplaceBack(from, index, Piece::PAWN);
            normalCaptureLeft &= (normalCaptureLeft - 1);
        }

        Bitboard promoteCaptureLeft = captureLeft & BACK_RANK_MASK;
        while (promoteCaptureLeft) {
            uint8_t index = std::countr_zero(promoteCaptureLeft);
            uint8_t from = index + shiftLeft;
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::QUEEN);
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::ROOK);
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::BISHOP);
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::KNIGHT);
            promoteCaptureLeft &= (promoteCaptureLeft - 1);
        }

        Bitboard normalCaptureRight = captureRight & ~BACK_RANK_MASK;
        while (normalCaptureRight) {
            uint8_t index = std::countr_zero(normalCaptureRight);
            uint8_t from = index + shiftRight;
            attacks.EmplaceBack(from, index, Piece::PAWN);
            normalCaptureRight &= (normalCaptureRight - 1);
        }

        Bitboard promoteCaptureRight = captureRight & BACK_RANK_MASK;
        while (promoteCaptureRight) {
            uint8_t index = std::countr_zero(promoteCaptureRight);
            uint8_t from = index + shiftRight;
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::QUEEN);
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::ROOK);
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::BISHOP);
            attacks.EmplaceBack(from, index, Piece::PAWN, Piece::KNIGHT);
            promoteCaptureRight &= (promoteCaptureRight - 1);
        }
    }

    template<Color::Value color> void FindPawnQuiets(const BoardState& state, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard RANK_3_MASK = 0b00000000'00000000'11111111'00000000'00000000'00000000'00000000'00000000;
        Bitboard RANK_6_MASK = 0b00000000'00000000'00000000'00000000'00000000'11111111'00000000'00000000;

        Bitboard pawns = state.pieces.Occupancy(color, Piece::PAWN);
        Bitboard occupancy = state.pieces.Occupancy();

        Bitboard singlePush;
        Bitboard doublePush;
        constexpr int8_t singleOffset = (color == Color::WHITE) ? 8 : -8;
        constexpr int8_t doubleOffset = (color == Color::WHITE) ? 16 : -16;
        if constexpr (color == Color::WHITE) {
            singlePush = (pawns >> 8) & ~occupancy;
            doublePush = ((singlePush & RANK_3_MASK) >> 8) & ~occupancy;
        } else if constexpr (color == Color::BLACK) {
            singlePush = (pawns << 8) & ~occupancy;
            doublePush = ((singlePush & RANK_6_MASK) << 8) & ~occupancy;
        } else {
            std::unreachable();
        }

        Bitboard singleNormal = singlePush & ~BACK_RANK_MASK;
        while (singleNormal) {
            uint8_t index = std::countr_zero(singleNormal);
            uint8_t from = index + singleOffset;
            quiets.EmplaceBack(from, index, Piece::PAWN);
            singleNormal &= (singleNormal - 1);
        }

        Bitboard singlePromote = singlePush & BACK_RANK_MASK;
        while (singlePromote) {
            uint8_t index = std::countr_zero(singlePromote);
            uint8_t from = index + singleOffset;
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::QUEEN);
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::ROOK);
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::BISHOP);
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::KNIGHT);
            singlePromote &= (singlePromote - 1);
        }

        // No promotions here
        while (doublePush) {
            uint8_t index = std::countr_zero(doublePush);
            uint8_t from = index + doubleOffset;
            quiets.EmplaceBack(from, index, Piece::PAWN);
            doublePush &= (doublePush - 1);
        }
    }

    template<Color::Value color> std::size_t CountPawnAttacks(const BoardState& state)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard A_FILE_MASK = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;
        Bitboard H_FILE_MASK = 0b10000000'10000000'10000000'10000000'10000000'10000000'10000000'10000000;

        uint64_t enPassantBit = (state.enPassantIndex != UINT8_MAX) ? 1ull << state.enPassantIndex : 0ull;

        Bitboard captureLeft;
        Bitboard captureRight;
        if constexpr (color == Color::WHITE) {
            Bitboard targetBits = state.pieces.Occupancy(Color::BLACK) | enPassantBit;
            Bitboard pawns = state.pieces.Occupancy(Color::WHITE, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) >> 9) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) >> 7)  & targetBits;
        } else if constexpr (color == Color::BLACK)  {
            Bitboard targetBits = state.pieces.Occupancy(Color::WHITE) | enPassantBit;
            Bitboard pawns = state.pieces.Occupancy(Color::BLACK, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) << 7) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) << 9)  & targetBits;
        } else {
            std::unreachable();
        }

        std::size_t captureCount = std::popcount(captureLeft) + std::popcount(captureRight);
        // Already counted the capture, add 3 more for promotions (adds up to 4 - one per piece)
        captureCount += 3 * (std::popcount(captureLeft & BACK_RANK_MASK) + std::popcount(captureRight & BACK_RANK_MASK));
        return captureCount;
    }

    template<Color::Value color> std::size_t CountPawnQuiets(const BoardState& state)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard RANK_3_MASK = 0b00000000'00000000'11111111'00000000'00000000'00000000'00000000'00000000;
        Bitboard RANK_6_MASK = 0b00000000'00000000'00000000'00000000'00000000'11111111'00000000'00000000;

        Bitboard pawns = state.pieces.Occupancy(color, Piece::PAWN);
        Bitboard occupancy = state.pieces.Occupancy();

        Bitboard singlePush;
        Bitboard doublePush;
        if constexpr (color == Color::WHITE) {
            singlePush = (pawns >> 8) & ~occupancy;
            doublePush = ((singlePush & RANK_3_MASK) >> 8) & ~occupancy;
        } else if constexpr (color == Color::BLACK) {
            singlePush = (pawns << 8) & ~occupancy;
            doublePush = ((singlePush & RANK_6_MASK) << 8) & ~occupancy;
        } else {
            std::unreachable();
        }

        std::size_t moveCount = std::popcount(singlePush) + std::popcount(doublePush);
        // Already counted the move to back rank, add 3 more for promotions (adds up to 4 - one per piece)
        moveCount += 3 * std::popcount(singlePush & BACK_RANK_MASK);
        return moveCount;
    }

    constexpr PawnAttackFunction attackFunctions[2] = {
        &FindPawnAttacks<Color::WHITE>,
        &FindPawnAttacks<Color::BLACK>
    };
    constexpr PawnQuietFunction quietFunctions[2] = {
        &FindPawnQuiets<Color::WHITE>,
        &FindPawnQuiets<Color::BLACK>
    };
    constexpr PawnCountFunction attackCountFunctions[2] = {
        &CountPawnAttacks<Color::WHITE>,
        &CountPawnAttacks<Color::BLACK>
    };
    constexpr PawnCountFunction quietCountFunctions[2] = {
        &CountPawnQuiets<Color::WHITE>,
        &CountPawnQuiets<Color::BLACK>
    };
};

namespace movegen {
    void FindAllAttacks(const BoardState& state, const AttackTable& attackTable, AttackMoveBuffer& attacks);
    void FindAllPawnAttacks(const BoardState& state, Color::Value color, AttackMoveBuffer& attacks);
    void FindKnightAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks);
    void FindKingAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks);
    void FindBishopAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks);
    void FindRookAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks);
    void FindQueenAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks);
    void FindSliderAttacks(uint8_t index, Piece::Value piece, Bitboard attackBits, AttackMoveBuffer& attacks);

    void FindAllQuiets(const BoardState& state, const AttackTable& attackTable, QuietMoveBuffer& quiets);
    void FindAllPawnQuiets(const BoardState& state, Color::Value color, QuietMoveBuffer& quiets);
    void FindKnightQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets);
    void FindKingQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets);
    void FindBishopQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets);
    void FindRookQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets);
    void FindQueenQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets);
    void FindSliderQuiets(uint8_t index, Piece::Value piece, Bitboard attackBits, QuietMoveBuffer& quiets);

    std::size_t CountAllAttacks(const BoardState& state, const AttackTable& attackTable);
    std::size_t CountAllQuiets(const BoardState& state, const AttackTable& attackTable);
    std::size_t CountAllPawnAttacks(const BoardState& state, Color::Value color);
    std::size_t CountAllPawnQuiets(const BoardState& state, Color::Value color);
    std::size_t CountKnightAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountKnightQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountBishopAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountBishopQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountRookAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountRookQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountQueenAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountQueenQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountKingAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index);
    std::size_t CountKingQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index);
};
