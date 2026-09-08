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

class Pawngen {
public:
    template<Color::Value color> static void FindPawnAttacks(const BoardState& state, AttackMoveBuffer& attacks)
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
            Bitboard targetBits = state.pieces.OccupancyMask(Color::BLACK) | enPassantBit;
            Bitboard pawns = state.pieces.OccupancyMask(Color::WHITE, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) >> 9) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) >> 7)  & targetBits;
        } else if constexpr (color == Color::BLACK)  {
            Bitboard targetBits = state.pieces.OccupancyMask(Color::WHITE) | enPassantBit;
            Bitboard pawns = state.pieces.OccupancyMask(Color::BLACK, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) << 7) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) << 9)  & targetBits;
        } else {
            std::unreachable();
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

    template<Color::Value color> static void FindPawnQuiets(const BoardState& state, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard RANK_3_MASK = 0b00000000'00000000'11111111'00000000'00000000'00000000'00000000'00000000;
        Bitboard RANK_6_MASK = 0b00000000'00000000'00000000'00000000'00000000'11111111'00000000'00000000;

        Bitboard pawns = state.pieces.OccupancyMask(color, Piece::PAWN);
        Bitboard occupancy = state.pieces.OccupancyMask();

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
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::KNIGHT);
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::BISHOP);
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::ROOK);
            quiets.EmplaceBack(from, index, Piece::PAWN, Piece::QUEEN);
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

    template<Color::Value color> static std::size_t CountPawnAttacks(const BoardState& state)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard A_FILE_MASK = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;
        Bitboard H_FILE_MASK = 0b10000000'10000000'10000000'10000000'10000000'10000000'10000000'10000000;

        uint64_t enPassantBit = (state.enPassantIndex != UINT8_MAX) ? 1ull << state.enPassantIndex : 0ull;

        Bitboard captureLeft;
        Bitboard captureRight;
        if constexpr (color == Color::WHITE) {
            Bitboard targetBits = state.pieces.OccupancyMask(Color::BLACK) | enPassantBit;
            Bitboard pawns = state.pieces.OccupancyMask(Color::WHITE, Piece::PAWN);
            captureLeft = ((pawns & ~A_FILE_MASK) >> 9) & targetBits;
            captureRight = ((pawns & ~H_FILE_MASK) >> 7)  & targetBits;
        } else if constexpr (color == Color::BLACK)  {
            Bitboard targetBits = state.pieces.OccupancyMask(Color::WHITE) | enPassantBit;
            Bitboard pawns = state.pieces.OccupancyMask(Color::BLACK, Piece::PAWN);
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

    template<Color::Value color> static std::size_t CountPawnQuiets(const BoardState& state)
    {
        JUPITER_TRACE();

        Bitboard BACK_RANK_MASK = 0b11111111'00000000'00000000'00000000'00000000'00000000'00000000'11111111;
        Bitboard RANK_3_MASK = 0b00000000'00000000'00000000'00000000'00000000'11111111'00000000'00000000;
        Bitboard RANK_6_MASK = 0b00000000'00000000'11111111'00000000'00000000'00000000'00000000'00000000;

        Bitboard pawns = state.pieces.OccupancyMask(color, Piece::PAWN);
        Bitboard occupancy = state.pieces.OccupancyMask();

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

public:
    static constexpr PawnAttackFunction attackFunctions[2] = {
        &Pawngen::FindPawnAttacks<Color::WHITE>,
        &Pawngen::FindPawnAttacks<Color::BLACK>
    };
    static constexpr PawnQuietFunction quietFunctions[2] = {
        &Pawngen::FindPawnQuiets<Color::WHITE>,
        &Pawngen::FindPawnQuiets<Color::BLACK>
    };
    static constexpr PawnCountFunction attackCountFunctions[2] = {
        &Pawngen::CountPawnAttacks<Color::WHITE>,
        &Pawngen::CountPawnAttacks<Color::BLACK>
    };
    static constexpr PawnCountFunction quietCountFunctions[2] = {
        &Pawngen::CountPawnQuiets<Color::WHITE>,
        &Pawngen::CountPawnQuiets<Color::BLACK>
    };
};

class Movegen {
public:
    Movegen(const BoardState& state, const AttackTable& attackTable)
        : m_State{std::forward<const BoardState>(state)}, m_AttackTable{std::forward<const AttackTable>(attackTable)} {}

    void FindAllAttacks(AttackMoveBuffer& attacks) const;
    void FindAllQuiets(QuietMoveBuffer& quiets) const;
    void FindAllPawnAttacks(Color::Value color, AttackMoveBuffer& attacks) const;
    void FindAllPawnQuiets(Color::Value color, QuietMoveBuffer& quiets) const;
    void FindKnightAttacks(uint8_t index, AttackMoveBuffer& attacks) const;
    void FindKnightQuiets(uint8_t index, QuietMoveBuffer& quiets) const;
    void FindKingAttacks(uint8_t index, AttackMoveBuffer& attacks) const;
    void FindKingQuiets(uint8_t index, QuietMoveBuffer& quiets) const;
    void FindBishopAttacks(uint8_t index, AttackMoveBuffer& attacks) const;
    void FindBishopQuiets(uint8_t index, QuietMoveBuffer& quiets) const;
    void FindRookAttacks(uint8_t index, AttackMoveBuffer& attacks) const;
    void FindRookQuiets(uint8_t index, QuietMoveBuffer& quiets) const;
    void FindQueenAttacks(uint8_t index, AttackMoveBuffer& attacks) const;
    void FindQueenQuiets(uint8_t index, QuietMoveBuffer& quiets) const;
    void FindSliderAttacks(uint8_t index, Piece::Value piece, Bitboard attackBits, AttackMoveBuffer& attacks) const;
    void FindSliderQuiets(uint8_t index, Piece::Value piece, Bitboard attackBits, QuietMoveBuffer& quiets) const;

    [[nodiscard]] std::size_t CountAllAttacks() const;
    [[nodiscard]] std::size_t CountAllQuiets() const;
    [[nodiscard]] std::size_t CountAllPawnAttacks(Color::Value color) const;
    [[nodiscard]] std::size_t CountAllPawnQuiets(Color::Value color) const;
    [[nodiscard]] std::size_t CountKnightAttacks(uint8_t index) const;
    [[nodiscard]] std::size_t CountKnightQuiets(uint8_t index) const;
    [[nodiscard]] std::size_t CountBishopAttacks(uint8_t index) const;
    [[nodiscard]] std::size_t CountBishopQuiets(uint8_t index) const;
    [[nodiscard]] std::size_t CountRookAttacks(uint8_t index) const;
    [[nodiscard]] std::size_t CountRookQuiets(uint8_t index) const;
    [[nodiscard]] std::size_t CountQueenAttacks(uint8_t index) const;
    [[nodiscard]] std::size_t CountQueenQuiets(uint8_t index) const;
    [[nodiscard]] std::size_t CountKingAttacks(uint8_t index) const;
    [[nodiscard]] std::size_t CountKingQuiets(uint8_t index) const;

private:
    const BoardState& m_State;
    const AttackTable& m_AttackTable;
};
