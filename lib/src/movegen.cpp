#include "movegen.h"
#include "core.h"
#include "instrumenter.h"
#include <bit>

Move Movegen::Stream(const Evaluator& evaluator, bool attackMode)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    switch (m_StreamState) {
        case StreamState::NONE:
            FindAttacks();
            m_StreamState = StreamState::GOOD_ATTACKS;
            if (m_BestMove.IsValid())
                return m_BestMove;
            // Fall through
        case StreamState::GOOD_ATTACKS:
            while (m_AttacksIndex < m_Attacks.Size()) {
                const Move& move = m_Attacks[m_AttacksIndex++];
                if (move != m_BestMove && evaluator.SEE(std::forward<const BoardState>(m_State), move) > 0)
                    return move;
                m_BadAttacks.PushBack(move);
            }
            if (!attackMode) {
                FindQuiets();
                m_StreamState = StreamState::QUIETS;
            }
            // Fall through
        case StreamState::QUIETS:
            if (!attackMode) {
                while (m_QuietsIndex < m_Quiets.Size()) {
                    const Move move = m_Quiets[m_QuietsIndex++];
                    if (move != m_BestMove)
                        return move;
                }
            }
            m_StreamState = StreamState::BAD_ATTACKS;
            m_AttacksIndex = 0;
            // Fall through
        case StreamState::BAD_ATTACKS:
            while (m_AttacksIndex < m_BadAttacks.Size()) {
                const Move move = m_BadAttacks[m_AttacksIndex++];
                if (move != m_BestMove)
                    return move;
            }
            m_StreamState = StreamState::FINISHED;
            // Fall through
        case StreamState::FINISHED:
            return Move::Invalid();
    }
}

const AttackMoveBuffer& Movegen::GetAttacks()
{
    FindAttacks();
    return m_Attacks;
}

const QuietMoveBuffer& Movegen::GetQuiets()
{
    FindQuiets();
    return m_Quiets;
}

void Movegen::FindAttacks()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_Attacks.Resize(0);

    // Queens
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindQueenAttacks(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Bishops
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindBishopAttacks(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Pawns
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::PAWN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindPawnAttacks(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Knights
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKnightAttacks(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Rooks
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindRookAttacks(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Kings
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKingAttacks(index);
            occupancy &= (occupancy - 1);
        }
    }

#ifdef DEBUG 
    for (const Move move : m_Attacks) {
        Color::Value attackerColor = m_State.pieces.PieceInSquare(move.from).first;
        Color::Value targetColor = m_State.pieces.PieceInSquare(move.to).first;
        if (targetColor != Color::Opposite(attackerColor)) {
            m_State.pieces.Dump();
            throw JupiterException(std::string("Attack move does not attack anything: ") + move.ToLAN().chars);
        }
    }
#endif
}

void Movegen::FindQuiets()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_Quiets.Resize(0);

    // Pawns
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::PAWN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindPawnQuiets(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Knights
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KNIGHT);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKnightQuiets(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Bishops
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::BISHOP);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindBishopQuiets(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Rooks
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::ROOK);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindRookQuiets(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Queens
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::QUEEN);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindQueenQuiets(index);
            occupancy &= (occupancy - 1);
        }
    }

    // Kings
    {
        Bitboard occupancy = m_State.pieces.OccupancyMask(m_State.turn, Piece::KING);
        while (occupancy) {
            uint8_t index = std::countr_zero(occupancy);
            FindKingQuiets(index);
            occupancy &= (occupancy - 1);
        }
    }
}

void Movegen::FindPawnAttacks(uint8_t index)
{
    JUPITER_TRACE();

    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back

    uint64_t enPassantBit = (m_State.enPassantIndex != UINT8_MAX) ? 1ul << m_State.enPassantIndex : 0ul;
    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::PAWN, m_State.turn, m_State.pieces.OccupancyMask());
    attacks &= (m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn)) | enPassantBit);
    
    while (attacks) {
        uint8_t toIndex = std::countr_zero(attacks);
        uint64_t toBit = 1ul << toIndex;
        if (toBit & backRankMask) {
            // Promotion
            m_Attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
            m_Attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
            m_Attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
            m_Attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
        } else {
            m_Attacks.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
        }
        attacks &= (attacks - 1);
    }
}

void Movegen::FindPawnQuiets(uint8_t index)
{
    JUPITER_TRACE();

    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back
    const uint64_t homeSquareMask = (m_State.turn == Color::WHITE) ? 0xFF000000000000 : 0x000000000000FF00;

    // In all cases, need to check if the move is a promotion

    int8_t delta = (m_State.turn == Color::WHITE) ? -8 : 8;
    uint8_t toIndex = index + delta;
    uint64_t toBit = 1ul << toIndex;
    if (!m_State.pieces.Has(toIndex)) {
        if (toBit & backRankMask) {
            // Promotion
            m_Quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
            m_Quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
            m_Quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
            m_Quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
        } else {
            m_Quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            toIndex += delta;
            if (((1ul << index) & homeSquareMask) && !m_State.pieces.Has(toIndex))
                m_Quiets.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
        }
    }
}

void Movegen::FindKnightAttacks(uint8_t index)
{
    JUPITER_TRACE();

    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, m_State.turn, 0);
    attacks &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attacks) {
        uint8_t toIndex = std::countr_zero(attacks);
        m_Attacks.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Movegen::FindKnightQuiets(uint8_t index)
{
    JUPITER_TRACE();

    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, m_State.turn, 0);
    attacks &= ~m_State.pieces.OccupancyMask();

    while (attacks) {
        uint8_t toIndex = std::countr_zero(attacks);
        m_Quiets.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Movegen::FindKingAttacks(uint8_t index)
{
    JUPITER_TRACE();

    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, m_State.turn, 0);
    attacks &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attacks) {
        uint8_t toIndex = std::countr_zero(attacks);
        m_Attacks.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Movegen::FindKingQuiets(uint8_t index)
{
    JUPITER_TRACE();

    // Attacks
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, m_State.turn, 0);
        attacks &= ~m_State.pieces.OccupancyMask();

        while (attacks) {
            uint8_t toIndex = std::countr_zero(attacks);
            m_Quiets.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    // Castling
    {
        if ((m_State.rights & CastlingRight::Kingside(m_State.turn)) && !m_State.pieces.HasAny({ index + 1ul, index + 2ul }))
            m_Quiets.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

        if ((m_State.rights & CastlingRight::Queenside(m_State.turn)) && !m_State.pieces.HasAny({ index - 1ul, index - 2ul, index - 3ul }))
            m_Quiets.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
    }
}

void Movegen::FindBishopAttacks(uint8_t index)
{
    JUPITER_TRACE();

    FindSliderAttacks(index, Piece::BISHOP);
}

void Movegen::FindBishopQuiets(uint8_t index)
{
    JUPITER_TRACE();

    FindSliderQuiets(index, Piece::BISHOP);
}

void Movegen::FindRookAttacks(uint8_t index)
{
    JUPITER_TRACE();

    FindSliderAttacks(index, Piece::ROOK);
}

void Movegen::FindRookQuiets(uint8_t index)
{
    JUPITER_TRACE();

    FindSliderQuiets(index, Piece::ROOK);
}

void Movegen::FindQueenAttacks(uint8_t index)
{
    JUPITER_TRACE();

    FindSliderAttacks(index, Piece::QUEEN);
}

void Movegen::FindQueenQuiets(uint8_t index)
{
    JUPITER_TRACE();

    FindSliderQuiets(index, Piece::QUEEN);
}

void Movegen::FindSliderAttacks(uint8_t index, Piece::Value piece)
{
    JUPITER_TRACE();

    Bitboard attacks = m_AttackTable.GetAttacks(index, piece, m_State.turn, m_State.pieces.OccupancyMask());
    attacks &= m_State.pieces.OccupancyMask(Color::Opposite(m_State.turn));

    while (attacks) {
        uint8_t toIndex = std::countr_zero(attacks);
        m_Attacks.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Movegen::FindSliderQuiets(uint8_t index, Piece::Value piece)
{
    JUPITER_TRACE();

    Bitboard attacks = m_AttackTable.GetAttacks(index, piece, m_State.turn, m_State.pieces.OccupancyMask());
    attacks &= ~m_State.pieces.OccupancyMask();

    while (attacks) {
        uint8_t toIndex = std::countr_zero(attacks);
        m_Quiets.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}
