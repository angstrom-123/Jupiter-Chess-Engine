#include "moveStream.h"
#include "core.h"
#include "util/instrumenter.h"
#include <algorithm>

Move MoveStream::Stream(bool quiescenceMode)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_LastWasQuiet = false;
    m_LastWasBadAttack = false;

    switch (m_StreamState) {
        case StreamState::NONE:
            m_Movegen.FindAllAttacks(m_Attacks);
            OrderAttacks();
            m_StreamState = StreamState::GOOD_ATTACKS;

            // Only return the best move if pseudolegal
            if (m_BestMove.IsValid() && m_State.pieces.PieceInSquare(m_BestMove.from).second == m_BestMove.piece)
                return m_BestMove;

            // Fall through
        case StreamState::GOOD_ATTACKS:
            while (m_AttacksIndex < m_Attacks.Size()) {
                const Move& move = m_Attacks[m_AttacksIndex++];
                if (move != m_BestMove) {
                    if (m_Eval->SEE(std::forward<const BoardState>(m_State), std::forward<const Move>(move)) > 0)
                        return move;
                    else 
                        m_BadAttacks.PushBack(move);
                }
            }

            if (quiescenceMode) {
                m_StreamState = StreamState::FINISHED;
                return Move::Invalid();
            } else {
                m_StreamState = StreamState::KILLERS;
            }

            // Fall through
        case StreamState::KILLERS:
            while (m_KillerIndex < m_Killers->Size()) {
                const Move& move = (*m_Killers)[m_KillerIndex++];

                // Only return the killer move if pseudolegal
                // Specifically don't set quiet move flag because killers are special cases
                if (move.IsValid() && m_State.pieces.PieceInSquare(move.from).second == move.piece)
                    return move;
            }

            m_Movegen.FindAllQuiets(m_Quiets);
            OrderQuiets();
            m_StreamState = StreamState::QUIETS;

            // Fall through
        case StreamState::QUIETS:
            while (m_QuietsIndex < m_Quiets.Size()) {
                const Move& move = m_Quiets[m_QuietsIndex++];
                if (move != m_BestMove && !m_Killers->Contains(move)) {
                    m_LastWasQuiet = true;
                    return move;
                }
            }

            m_StreamState = StreamState::BAD_ATTACKS;
            m_AttacksIndex = 0;

            // Fall through
        case StreamState::BAD_ATTACKS:
            while (m_AttacksIndex < m_Attacks.Size()) {
                const Move& move = m_Attacks[m_AttacksIndex++];
                if (move != m_BestMove) {
                    m_LastWasBadAttack = false;
                    return move;
                }
            }

            m_StreamState = StreamState::FINISHED;

            // Fall through
        case StreamState::FINISHED:
            return Move::Invalid();
    };
}

static const uint8_t MVV_LVA_TABLE[Piece::MAX_ENUM + 1][Piece::MAX_ENUM + 1] = {
    { 10, 11, 12, 13, 14, 15, 0 }, // victim P, attacker K, Q, R, B, N, P, None
    { 20, 21, 22, 23, 24, 25, 0 }, // victim N, attacker K, Q, R, B, N, P, None
    { 30, 31, 32, 33, 34, 35, 0 }, // victim B, attacker K, Q, R, B, N, P, None
    { 40, 41, 42, 43, 44, 45, 0 }, // victim R, attacker K, Q, R, B, N, P, None
    { 50, 51, 52, 53, 54, 55, 0 }, // victim Q, attacker K, Q, R, B, N, P, None
    { 0, 0, 0, 0, 0, 0, 0 },       // victim K, attacker K, Q, R, B, N, P, None
    { 0, 0, 0, 0, 0, 0, 0},        // victim _, attacker K, Q, R, B, N, P, None
};

void MoveStream::OrderAttacks()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    std::sort(m_Attacks.begin(), m_Attacks.end(), [this](const Move& a, const Move& b) {
        Piece::Value victimA = m_State.pieces.PieceInSquare(a.to).second;
        if (!Piece::IsValid(victimA)) // In case of en passant
            victimA = Piece::PAWN;
        Piece::Value aggressorA = m_State.pieces.PieceInSquare(a.from).second;
        uint8_t scoreA = MVV_LVA_TABLE[victimA][aggressorA];

        Piece::Value victimB = m_State.pieces.PieceInSquare(b.to).second;
        if (!Piece::IsValid(victimB)) // In case of en passant
            victimB = Piece::PAWN;
        Piece::Value aggressorB = m_State.pieces.PieceInSquare(b.from).second;
        uint8_t scoreB = MVV_LVA_TABLE[victimB][aggressorB];

        return scoreA > scoreB;
    });
}

// History heuristic
void MoveStream::OrderQuiets()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    std::sort(m_Quiets.begin(), m_Quiets.end(), [this](const Move& a, const Move& b) {
        return *m_HistoryTable[a.from][a.to] > *m_HistoryTable[b.from][b.to];
    });
}
