#pragma once

#include "datastructure/twoStack.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "evaluation/evaluator.h"
#include <cstdint>

// Not quite the theoretical maximums but I don't anticipate this becoming a problem
const uint8_t MAX_PLY = 32;
const uint64_t MAX_KILLERS = 2;

const int16_t MAX_HISTORY = INT16_MAX;

using KillerMoveStack = TwoStack<Move>; // Special circular stack for just 2 elements, works fine for now
static_assert(MAX_KILLERS == 2 && "Can't use TwoStack for KillerMoveStack if there are not 2 killers");
using KillerMoveBuffer = KillerMoveStack[MAX_PLY + 1]; // + 1 to store an empty stack to pass in if ply exceeds MAX_PLY
using HistoryTable = int16_t[Color::MAX_ENUM][64][64];

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

using StreamFlags = uint8_t;
struct StreamFlag {
    typedef enum : uint8_t {
        NONE                = 0x0,
        QUIESCENCE          = 0x1,
        LAST_WAS_QUIET      = 0x2,
        LAST_WAS_BAD_ATTACK = 0x4,
    } Value;
};

class MoveStream {
public:
    MoveStream() = default;
    void Init(const BoardState& state, const AttackTable& attackTable, const KillerMoveStack& killers, bool quiescenceMode);
    Move Stream(const BoardState& state, const Evaluator& eval, const HistoryTable& historyTable);

    INLINE const AttackMoveBuffer& GetAttacks() const { return m_Attacks; }
    INLINE const QuietMoveBuffer& GetQuiets() const 
    { 
        #ifdef DEBUG
        if (m_Flags & StreamFlag::QUIESCENCE) 
            ERROR("Getting quiets from movestream when in quiescence mode"); 
        #endif
        return m_Quiets;
    }
    INLINE void SetTTMove(Move move) { m_BestMove = move; }
    INLINE bool IsQuietMove() const { return m_Flags & StreamFlag::LAST_WAS_QUIET; }
    INLINE bool IsBadAttack() const { return m_Flags & StreamFlag::LAST_WAS_BAD_ATTACK; }
    INLINE bool IsFirstMove() const { return m_MoveCounter == 1; }
    INLINE uint8_t PseudoLegalCount() const { return m_Attacks.Size() + m_Quiets.Size(); }
    INLINE uint8_t PseudoLegalsRemaining() const { return PseudoLegalCount() - m_MoveCounter; }

private:
    void ScoreAttacks(const BoardState& state);
    Move PopOrderedAttack();
    void OrderQuiets(const HistoryTable& historyTable, Color::Value turn);

private:
    StreamState::Value m_StreamState{StreamState::NONE};
    StreamFlags m_Flags{0};
    uint8_t m_MoveCounter{0};
    uint8_t m_AttacksIndex{0};
    uint8_t m_BadAttacksIndex{0};
    uint8_t m_QuietsIndex{0};
    Buffer<uint8_t, MAX_ATTACKS> m_AttackScores;
    AttackMoveBuffer m_Attacks;
    QuietMoveBuffer m_Quiets;
    KillerMoveStack m_Killers;
    Move m_BestMove{Move::Invalid()};
};
