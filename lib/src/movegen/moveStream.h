#pragma once

#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "movegen/movegen.h"
#include "board/boardState.h"
#include "datastructure/buffer.h"
#include "evaluation/evaluator.h"
#include <cstdint>

// Not quite the theoretical maximums but I don't anticipate this becoming a problem
const uint64_t MAX_PLY = 32;
const uint64_t MAX_KILLERS = 2;

using KillerMoveBuffer = Buffer<Move, MAX_KILLERS>;
using HistoryTable = uint16_t[64][64];

struct StreamState {
    typedef enum : uint8_t {
        NONE,
        GOOD_ATTACKS,
        KILLERS,
        QUIETS,
        BAD_ATTACKS,
        FINISHED
    } Value;
};

class MoveStream {
public:
    // Full initialisation - contains all info for good move ordering (able to stream)
    // History table and killer moves are optional if you just want movegen but will segfault if streaming without
    MoveStream(
            const BoardState& state, 
            const AttackTable& attackTable, 
            const Evaluator *_Nullable eval = nullptr, 
            const HistoryTable *_Nullable historyTable = nullptr, 
            const KillerMoveBuffer *_Nullable killers = nullptr,
            Move bestMove = Move::Invalid()
    ) 
        : 
            m_State{std::forward<const BoardState>(state)}, 
            m_Movegen{Movegen(std::forward<const BoardState>(state), std::forward<const AttackTable>(attackTable))},
            m_Eval{eval},
            m_HistoryTable{historyTable},
            m_Killers{killers}, 
            m_BestMove{bestMove} {}

    Move Stream(bool attackMode = false);
    bool LastWasQuiet() const { return m_LastWasQuiet; }
    bool LastWasBadAttack() const { return m_LastWasBadAttack; }

private:
    void OrderAttacks();
    void OrderQuiets();

private:
    StreamState::Value m_StreamState{StreamState::NONE};
    const BoardState& m_State;
    Movegen m_Movegen;
    const Evaluator *_Nullable m_Eval{nullptr};
    const HistoryTable *_Nullable m_HistoryTable{nullptr};
    const KillerMoveBuffer *_Nullable m_Killers{nullptr};
    Move m_BestMove{Move::Invalid()};;
    AttackMoveBuffer m_Attacks;
    QuietMoveBuffer m_Quiets;
    AttackMoveBuffer m_BadAttacks;
    std::size_t m_AttacksIndex{0};
    std::size_t m_QuietsIndex{0};
    std::size_t m_KillerIndex{0};
    bool m_LastWasQuiet{false};
    bool m_LastWasBadAttack{false};
};
