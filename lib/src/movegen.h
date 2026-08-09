#pragma once

#include "attackTable.h"
#include "boardState.h"
#include "buffer.h"
#include "move.h"
#include <cstdint>

// Not quite the theoretical maximums but I don't anticipate this becoming a problem
const uint64_t MAX_POSSIBLE_QUIETS = 100;
const uint64_t MAX_POSSIBLE_ATTACKS = 100;

using QuietMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS>;
using AttackMoveBuffer = Buffer<Move, MAX_POSSIBLE_ATTACKS>;
using CombinedMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS + MAX_POSSIBLE_ATTACKS>;

namespace movegen {
    void FindAttacks(const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindQuiets(const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);

    void FindPawnAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindPawnQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer); // Doesn't use attack table
    void FindKnightAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindKnightQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);
    void FindKingAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindKingQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);
    void FindBishopAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindBishopQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);
    void FindRookAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindRookQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);
    void FindQueenAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindQueenQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);
    void FindSliderAttacks(uint8_t index, Piece::Value piece, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer);
    void FindSliderQuiets(uint8_t index, Piece::Value piece, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer);
}
