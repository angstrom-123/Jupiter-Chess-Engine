#include "moveStream.h"
#include "core.h"
#include "movegen/movegen.h"
#include "util/instrumenter.h"
#include <algorithm>

void MoveStream::Init(const BoardState& state, const AttackTable& attackTable, const KillerMoveStack& killers, bool quiescenceMode)
{
    m_Flags = quiescenceMode ? StreamFlag::QUIESCENCE : StreamFlag::NONE;
    m_Killers = killers;

    movegen::FindAllAttacks(state, attackTable, m_Attacks);
    if (!quiescenceMode)
        movegen::FindAllQuiets(state, attackTable, m_Quiets);
}

Move MoveStream::Stream(const BoardState& state, const Evaluator& eval, const HistoryTable& historyTable)
{
    m_Flags ^= StreamFlag::LAST_WAS_QUIET | StreamFlag::LAST_WAS_BAD_ATTACK;

    switch (m_StreamState) {
        case StreamState::NONE:
        {
            JUPITER_TRACE();

            ScoreAttacks(state);
            m_StreamState = StreamState::GOOD_ATTACKS;

            // Only return the best move if pseudolegal
            if (m_BestMove.IsValid()) {
                m_MoveCounter++;
                return m_BestMove;
            }

            // Fall through
        }
        case StreamState::GOOD_ATTACKS: 
        {
            JUPITER_TRACE();

            Move move;
            while ((move = PopOrderedAttack()).IsValid()) {
                if (move != m_BestMove) {
                    if (eval.SEE(state, move) > 0) {
                        m_MoveCounter++;
                        return move;
                    } else {
                        // Overwrite the start of the buffer with the bad moves so that I can avoid 
                        // having a separate buffer just for the bad moves. This saves some bytes.
                        m_Attacks[m_BadAttacksIndex++] = move;
                    }
                }
            }

            if (m_Flags & StreamFlag::QUIESCENCE) {
                m_StreamState = StreamState::FINISHED;
                return Move::Invalid();
            } else {
                m_StreamState = StreamState::KILLERS;
            }

            // Fall through
        }
        case StreamState::KILLERS:
        {
            JUPITER_TRACE();

            while (m_QuietsIndex < m_Killers.Size()) {
                const Move move = m_Killers.Peek(m_QuietsIndex++);

                if (state.IsPseudoLegal(GetAttacks(), GetQuiets(), move)) {
                    // Don't set quiet move flag because killers are special cases
                    m_MoveCounter++;
                    return move;
                }
            }

            OrderQuiets(historyTable, state.turn);
            m_StreamState = StreamState::QUIETS;
            m_QuietsIndex = 0;

            // Fall through
        }
        case StreamState::QUIETS:
        {
            JUPITER_TRACE();

            while (m_QuietsIndex < m_Quiets.Size()) {
                Move move = m_Quiets[m_QuietsIndex++];
                if (move != m_BestMove && !m_Killers.Contains(move)) {
                    m_Flags |= StreamFlag::LAST_WAS_QUIET;
                    m_MoveCounter++;
                    return move;
                }
            }

            m_StreamState = StreamState::BAD_ATTACKS;
            m_AttacksIndex = 0;

            // Fall through
        }
        case StreamState::BAD_ATTACKS:
        {
            JUPITER_TRACE();

            while (m_AttacksIndex < m_BadAttacksIndex) {
                const Move move = m_Attacks[m_AttacksIndex++];
                if (move != m_BestMove) {
                    m_Flags |= StreamFlag::LAST_WAS_BAD_ATTACK;
                    m_MoveCounter++;
                    return move;
                }
            }

            m_StreamState = StreamState::FINISHED;

            // Fall through
        }
        case StreamState::FINISHED:
        {
            return Move::Invalid();
        }
    };

    std::unreachable();
}

static const uint8_t MVV_LVA_TABLE[Piece::MAX_ENUM + 1][Piece::MAX_ENUM + 1] = {
    { 15, 14, 13, 12, 11, 10, 0 }, // victim P, attacker P, N, B, R, Q, K, None
    { 25, 24, 23, 22, 21, 20, 0 }, // victim N, attacker P, N, B, R, Q, K, None
    { 35, 34, 33, 32, 31, 30, 0 }, // victim B, attacker P, N, B, R, Q, K, None
    { 45, 44, 43, 42, 41, 40, 0 }, // victim R, attacker P, N, B, R, Q, K, None
    { 55, 54, 53, 52, 51, 50, 0 }, // victim Q, attacker P, N, B, R, Q, K, None
    { 0, 0, 0, 0, 0, 0, 0 },       // victim K, attacker P, N, B, R, Q, K, None
    { 0, 0, 0, 0, 0, 0, 0},        // victim _, attacker P, N, B, R, Q, K, None
};
static const uint8_t MVV_LVA_MAX = 55;

void MoveStream::ScoreAttacks(const BoardState& state)
{
    JUPITER_TRACE();

    Color::Value enemy = Color::Opposite(state.turn);
    for (const Move move : m_Attacks) {
        Piece::Value victim = state.pieces.PieceInSquare(enemy, move.to);
        if (!Piece::IsValid(victim)) // In case of en passant
            victim = Piece::PAWN;
        Piece::Value aggressor = state.pieces.PieceInSquare(state.turn, move.from);
        m_AttackScores.PushBack(MVV_LVA_TABLE[victim][aggressor]);
    }
}

// Attack move MVV_LVA scores were all computed after generating them and stored packed in the same 
// uint32_t (lower 18 bits = move, following 8 bits = score). 
// This function acts as one iteration of non-increasing selection sort and swaps the best scoring
// remaining attack to the current index into the buffer. This should be quiet efficient because it 
// avoids sorting the entire buffer and we will often terminate early.
Move MoveStream::PopOrderedAttack()
{
    JUPITER_TRACE();

    // Scan forward to find the maximum scoring move from our starting index
    uint8_t bestScore = 0;
    std::size_t bestIndex = SIZE_MAX;
    for (std::size_t i = m_AttacksIndex; i < m_AttackScores.Size(); i++) {
        uint8_t score = m_AttackScores[i];
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
            if (bestScore == MVV_LVA_MAX)
                break;
        }
    }

    // No more moves remaining
    if (bestIndex == SIZE_MAX)
        return Move::Invalid();

    // Swap the best scoring move to the front and move its score back (lazy selection sort)
    Move best = m_Attacks[bestIndex];
    m_Attacks[bestIndex] = m_Attacks[m_AttacksIndex];
    m_Attacks[m_AttacksIndex] = best;
    m_AttackScores[bestIndex] = m_AttackScores[m_AttacksIndex];

    m_AttacksIndex++;

    return best;
}

void MoveStream::OrderQuiets(const HistoryTable& historyTable, Color::Value turn)
{
    JUPITER_TRACE();

    // So few that it really doesn't matter
    if (m_Quiets.Size() <= 4)
        return;

    std::sort(m_Quiets.begin(), m_Quiets.end(), [&historyTable, turn](const Move a, const Move b) {
        // History heuristic
        return historyTable[turn][a.from][a.to] > historyTable[turn][b.from][b.to];
    });
}
