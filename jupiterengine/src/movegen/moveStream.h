#pragma once

#include "movegen/move.h"
#include "movegen/movegen.h"
#include "board/boardState.h"
#include "evaluation/evaluator.h"
#include <cstdint>
#include "datastructure/circularStack.h"

#ifndef __clang__
    #include "nullables.h"
#endif

// Not quite the theoretical maximums but I don't anticipate this becoming a problem
const uint8_t MAX_PLY = 32;
const uint64_t MAX_KILLERS = 2;

using KillerMoveStack = CircularStack<Move, MAX_KILLERS>;
using KillerMoveBuffer = CircularStack<Move, MAX_KILLERS>[MAX_PLY + 1]; // + 1 to store an empty stack to pass in if ply exceeds MAX_PLY
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
            const Evaluator& eval, 
            const HistoryTable& historyTable, 
            const KillerMoveStack& killers,
            bool quiescenceMode
    ) 
        : 
            m_QuiescenceMode{quiescenceMode},
            m_State{std::forward<const BoardState>(state)}, 
            m_Eval{eval},
            m_HistoryTable{historyTable},
            m_Killers{KillerMoveStack(killers)} // Copy, should be ok since it is small (32 bytes for 2 killers)
    {
        movegen::FindAllAttacks(std::forward<const BoardState>(state), std::forward<const AttackTable>(attackTable), m_Attacks);
        if (!quiescenceMode)
            movegen::FindAllQuiets(std::forward<const BoardState>(state), std::forward<const AttackTable>(attackTable), m_Quiets);
    }

    const AttackMoveBuffer& GetAttacks() const { return m_Attacks; }
    const QuietMoveBuffer& GetQuiets() const 
    { 
        // Be careful
        if (m_QuiescenceMode) 
            ERROR("Getting quiets from movestream when in quiescence mode"); 
        return m_Quiets;
    }
    void SetTTMove(Move move) { m_BestMove = move; }
    Move Stream();
    bool IsQuietMove() const { return m_LastWasQuiet; }
    bool IsBadAttack() const { return m_LastWasBadAttack; }
    bool IsFirstMove() const { return m_MoveCounter == 1; }
    uint8_t GetMoveCount() const { return m_MoveCounter; }

private:
    void ScoreAttacks();
    Move PopOrderedAttack();
    void OrderQuiets();

private:
    StreamState::Value m_StreamState{StreamState::NONE};

    bool m_QuiescenceMode;
    const BoardState& m_State;
    const Evaluator& m_Eval;
    const HistoryTable& m_HistoryTable;
    KillerMoveStack m_Killers;

    Move m_BestMove{Move::Invalid()};
    Buffer<uint8_t, MAX_ATTACKS> m_AttackScores;
    AttackMoveBuffer m_Attacks;
    QuietMoveBuffer m_Quiets;
    AttackMoveBuffer m_BadAttacks;

    uint8_t m_AttacksIndex{0};
    uint8_t m_QuietsIndex{0};
    uint8_t m_KillerIndex{0};

    bool m_LastWasQuiet{false};
    bool m_LastWasBadAttack{false};
    uint8_t m_MoveCounter{0};
};
