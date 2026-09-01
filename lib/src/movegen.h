#pragma once

#include "attackTable.h"
#include "boardState.h"
#include "buffer.h"
#include "evaluator.h"
#include "move.h"
#include <cstdint>

// Not quite the theoretical maximums but I don't anticipate this becoming a problem
const uint64_t MAX_POSSIBLE_QUIETS = 100;
const uint64_t MAX_POSSIBLE_ATTACKS = 100;
const uint64_t MAX_PLY = 32;
const uint64_t MAX_KILLERS = 2;

using QuietMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS>;
using AttackMoveBuffer = Buffer<Move, MAX_POSSIBLE_ATTACKS>;
using CombinedMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS + MAX_POSSIBLE_ATTACKS>;
using KillerMoveBuffer = Buffer<Move[MAX_KILLERS], MAX_PLY>;

struct StreamState {
    typedef enum : uint8_t {
        NONE,
        GOOD_ATTACKS,
        QUIETS,
        BAD_ATTACKS,
        FINISHED
    } Value;
};

class Movegen {
public:
    Movegen(const BoardState& state, const AttackTable& attackTable, const Move (& killers)[2], Move bestMove = Move::Invalid())
        : 
            m_State{std::forward<const BoardState>(state)}, 
            m_AttackTable{std::forward<const AttackTable>(attackTable)}, 
            m_Killers{std::forward<const Move[2]>(killers)}, 
            m_BestMove{bestMove} {}
    Move Stream(const Evaluator& evaluator, bool attackMode = false);
    bool LastWasQuiet() const;
    std::size_t AttackCount();
    std::size_t QuietCount();

private:
    void OrderAttacks();
    void FindAttacks();
    void FindQuiets();
    void FindPawnAttacks(uint8_t index);
    void FindPawnQuiets(uint8_t index); 
    void FindKnightAttacks(uint8_t index);
    void FindKnightQuiets(uint8_t index);
    void FindKingAttacks(uint8_t index);
    void FindKingQuiets(uint8_t index);
    void FindBishopAttacks(uint8_t index);
    void FindBishopQuiets(uint8_t index);
    void FindRookAttacks(uint8_t index);
    void FindRookQuiets(uint8_t index);
    void FindQueenAttacks(uint8_t index);
    void FindQueenQuiets(uint8_t index);
    void FindSliderAttacks(uint8_t index, Piece::Value piece);
    void FindSliderQuiets(uint8_t index, Piece::Value piece);

private:
    StreamState::Value m_StreamState{StreamState::NONE};
    const BoardState& m_State;
    const AttackTable& m_AttackTable;
    const Move (& m_Killers)[2];
    Move m_BestMove{Move::Invalid()};;
    AttackMoveBuffer m_Attacks;
    QuietMoveBuffer m_Quiets;
    AttackMoveBuffer m_BadAttacks;
    std::size_t m_AttacksIndex{0};
    std::size_t m_QuietsIndex{0};
    std::size_t m_KillerIndex{0};
    bool m_LastWasQuiet{false};
};
