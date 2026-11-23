#include "movegen.h"
#include "board/bitboard.h"
#include "core.h"
#include "movegen/move.h"
#include "util/instrumenter.h"
#include <bit>

namespace movegen {
    void FindAllPawnAttacks(const BoardState& state, Color::Value color, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        pawngen::attackFunctions[color](state, attacks);
    }

    void FindKnightAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KnightAttacks(index);
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            attacks.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }

    void FindKingAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KingAttacks(index);
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            attacks.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }

    void FindSliderAttacks(uint8_t index, Piece::Value piece, Bitboard attackBits, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            attacks.EmplaceBack(index, toIndex, piece, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }

    void FindBishopAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.BishopAttacks(index, state.pieces.Occupancy());
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        FindSliderAttacks(index, Piece::BISHOP, attackBits, attacks);
    }

    void FindRookAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.RookAttacks(index, state.pieces.Occupancy());
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        FindSliderAttacks(index, Piece::ROOK, attackBits, attacks);
    }

    void FindQueenAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.QueenAttacks(index, state.pieces.Occupancy());
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        FindSliderAttacks(index, Piece::QUEEN, attackBits, attacks);
    }

    void FindAllAttacks(const BoardState& state, const AttackTable& attackTable, AttackMoveBuffer& attacks)
    {
        JUPITER_TRACE();

        Bitboard queens = state.pieces.Occupancy(state.turn, Piece::QUEEN);
        while (queens) {
            uint8_t index = std::countr_zero(queens);
            FindQueenAttacks(state, attackTable, index, attacks);
            queens &= (queens - 1);
        }

        Bitboard rooks = state.pieces.Occupancy(state.turn, Piece::ROOK);
        while (rooks) {
            uint8_t index = std::countr_zero(rooks);
            FindRookAttacks(state, attackTable, index, attacks);
            rooks &= (rooks - 1);
        }

        Bitboard bishops = state.pieces.Occupancy(state.turn, Piece::BISHOP);
        while (bishops) {
            uint8_t index = std::countr_zero(bishops);
            FindBishopAttacks(state, attackTable, index, attacks);
            bishops &= (bishops - 1);
        }

        Bitboard knights = state.pieces.Occupancy(state.turn, Piece::KNIGHT);
        while (knights) {
            uint8_t index = std::countr_zero(knights);
            FindKnightAttacks(state, attackTable, index, attacks);
            knights &= (knights - 1);
        }

        FindAllPawnAttacks(state, state.turn, attacks);

        Bitboard kings = state.pieces.Occupancy(state.turn, Piece::KING);
        while (kings) {
            uint8_t index = std::countr_zero(kings);
            FindKingAttacks(state, attackTable, index, attacks);
            kings &= (kings - 1);
        }
    }

    void FindAllQuiets(const BoardState& state, const AttackTable& attackTable, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard queens;
        queens = state.pieces.Occupancy(state.turn, Piece::QUEEN);
        while (queens) {
            uint8_t index = std::countr_zero(queens);
            FindQueenQuiets(state, attackTable, index, quiets);
            queens &= (queens - 1);
        }

        Bitboard rooks = state.pieces.Occupancy(state.turn, Piece::ROOK);
        while (rooks) {
            uint8_t index = std::countr_zero(rooks);
            FindRookQuiets(state, attackTable, index, quiets);
            rooks &= (rooks - 1);
        }

        Bitboard bishops = state.pieces.Occupancy(state.turn, Piece::BISHOP);
        while (bishops) {
            uint8_t index = std::countr_zero(bishops);
            FindBishopQuiets(state, attackTable, index, quiets);
            bishops &= (bishops - 1);
        }

        Bitboard knights = state.pieces.Occupancy(state.turn, Piece::KNIGHT);
        while (knights) {
            uint8_t index = std::countr_zero(knights);
            FindKnightQuiets(state, attackTable, index, quiets);
            knights &= (knights - 1);
        }

        FindAllPawnQuiets(state, state.turn, quiets);

        Bitboard kings = state.pieces.Occupancy(state.turn, Piece::KING);
        while (kings) {
            uint8_t index = std::countr_zero(kings);
            FindKingQuiets(state, attackTable, index, quiets);
            kings &= (kings - 1);
        }
    }

    void FindAllPawnQuiets(const BoardState& state, Color::Value color, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();
        pawngen::quietFunctions[color](state, quiets);
    }

    void FindKnightQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KnightAttacks(index);
        attackBits &= ~state.pieces.Occupancy();

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            quiets.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }

    void FindKingQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KingAttacks(index);
        attackBits &= ~state.pieces.Occupancy();

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            quiets.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }

        // Castling
        if ((state.rights & CastlingRight::Kingside(state.turn)) && !state.pieces.Has(index + 1) && !state.pieces.Has(index + 2))
            quiets.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

        if ((state.rights & CastlingRight::Queenside(state.turn)) && !state.pieces.Has(index - 1) && !state.pieces.Has(index - 2) && !state.pieces.Has(index - 3))
            quiets.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
    }

    void FindBishopQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.BishopAttacks(index, state.pieces.Occupancy());
        attackBits &= ~state.pieces.Occupancy();
        FindSliderQuiets(index, Piece::BISHOP, attackBits, quiets);
    }

    void FindRookQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.RookAttacks(index, state.pieces.Occupancy());
        attackBits &= ~state.pieces.Occupancy();
        FindSliderQuiets(index, Piece::ROOK, attackBits, quiets);
    }

    void FindQueenQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.QueenAttacks(index, state.pieces.Occupancy());
        attackBits &= ~state.pieces.Occupancy();
        FindSliderQuiets(index, Piece::QUEEN, attackBits, quiets);
    }

    std::size_t CountAllAttacks(const BoardState& state, const AttackTable& attackTable)
    {
        JUPITER_TRACE();

        std::size_t total = 0;

        Bitboard queens = state.pieces.Occupancy(state.turn, Piece::QUEEN);
        while (queens) {
            uint8_t index = std::countr_zero(queens);
            total += CountQueenAttacks(state, attackTable, index);
            queens &= (queens - 1);
        }


        Bitboard rooks = state.pieces.Occupancy(state.turn, Piece::ROOK);
        while (rooks) {
            uint8_t index = std::countr_zero(rooks);
            total += CountRookAttacks(state, attackTable, index);
            rooks &= (rooks - 1);
        }

        Bitboard bishops = state.pieces.Occupancy(state.turn, Piece::BISHOP);
        while (bishops) {
            uint8_t index = std::countr_zero(bishops);
            total += CountBishopAttacks(state, attackTable, index);
            bishops &= (bishops - 1);
        }

        Bitboard knights = state.pieces.Occupancy(state.turn, Piece::KNIGHT);
        while (knights) {
            uint8_t index = std::countr_zero(knights);
            total += CountKnightAttacks(state, attackTable, index);
            knights &= (knights - 1);
        }

        total += CountAllPawnAttacks(state, state.turn);

        Bitboard kings = state.pieces.Occupancy(state.turn, Piece::KING);
        while (kings) {
            uint8_t index = std::countr_zero(kings);
            total += CountKingAttacks(state, attackTable, index);
            kings &= (kings - 1);
        }

        return total;
    }

    std::size_t CountAllQuiets(const BoardState& state, const AttackTable& attackTable)
    {
        JUPITER_TRACE();

        std::size_t total = 0;

        Bitboard queens = state.pieces.Occupancy(state.turn, Piece::QUEEN);
        while (queens) {
            uint8_t index = std::countr_zero(queens);
            total += CountQueenQuiets(state, attackTable, index);
            queens &= (queens - 1);
        }


        Bitboard rooks = state.pieces.Occupancy(state.turn, Piece::ROOK);
        while (rooks) {
            uint8_t index = std::countr_zero(rooks);
            total += CountRookQuiets(state, attackTable, index);
            rooks &= (rooks - 1);
        }

        Bitboard bishops = state.pieces.Occupancy(state.turn, Piece::BISHOP);
        while (bishops) {
            uint8_t index = std::countr_zero(bishops);
            total += CountBishopQuiets(state, attackTable, index);
            bishops &= (bishops - 1);
        }

        Bitboard knights = state.pieces.Occupancy(state.turn, Piece::KNIGHT);
        while (knights) {
            uint8_t index = std::countr_zero(knights);
            total += CountKnightQuiets(state, attackTable, index);
            knights &= (knights - 1);
        }

        total += CountAllPawnQuiets(state, state.turn);

        Bitboard kings = state.pieces.Occupancy(state.turn, Piece::KING);
        while (kings) {
            uint8_t index = std::countr_zero(kings);
            total += CountKingQuiets(state, attackTable, index);
            kings &= (kings - 1);
        }

        return total;
    }

    std::size_t CountAllPawnAttacks(const BoardState& state, Color::Value color)
    {
        return pawngen::attackCountFunctions[color](state);
    }

    std::size_t CountAllPawnQuiets(const BoardState& state, Color::Value color)
    {
        return pawngen::quietCountFunctions[color](state);
    }

    std::size_t CountKnightAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KnightAttacks(index);
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        return std::popcount(attackBits);
    }

    std::size_t CountKnightQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KnightAttacks(index);
        attackBits &= ~state.pieces.Occupancy();
        return std::popcount(attackBits);
    }

    std::size_t CountBishopAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.BishopAttacks(index, state.pieces.Occupancy());
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        return std::popcount(attackBits);
    }

    std::size_t CountBishopQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.BishopAttacks(index, state.pieces.Occupancy());
        attackBits &= ~state.pieces.Occupancy();
        return std::popcount(attackBits);
    }

    std::size_t CountRookAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.RookAttacks(index, state.pieces.Occupancy());
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        return std::popcount(attackBits);
    }

    std::size_t CountRookQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.RookAttacks(index, state.pieces.Occupancy());
        attackBits &= ~state.pieces.Occupancy();
        return std::popcount(attackBits);
    }

    std::size_t CountQueenAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.QueenAttacks(index, state.pieces.Occupancy());
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        return std::popcount(attackBits);
    }

    std::size_t CountQueenQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.QueenAttacks(index, state.pieces.Occupancy());
        attackBits &= ~state.pieces.Occupancy();
        return std::popcount(attackBits);
    }

    std::size_t CountKingAttacks(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        Bitboard attackBits = attackTable.KingAttacks(index);
        attackBits &= state.pieces.Occupancy(Color::Opposite(state.turn));
        return std::popcount(attackBits);
    }

    std::size_t CountKingQuiets(const BoardState& state, const AttackTable& attackTable, uint8_t index)
    {
        JUPITER_TRACE();

        std::size_t total = 0;

        Bitboard attackBits = attackTable.KingAttacks(index);
        attackBits &= ~state.pieces.Occupancy();
        total += std::popcount(attackBits);

        if ((state.rights & CastlingRight::Kingside(state.turn)) && !state.pieces.Has(index + 1) && !state.pieces.Has(index + 2))
            total++;

        if ((state.rights & CastlingRight::Queenside(state.turn)) && !state.pieces.Has(index - 1) && !state.pieces.Has(index - 2) && !state.pieces.Has(index - 3))
            total++;

        return total;
    }

    void FindSliderQuiets(uint8_t index, Piece::Value piece, Bitboard attackBits, QuietMoveBuffer& quiets)
    {
        JUPITER_TRACE();

        while (attackBits) {
            uint8_t toIndex = std::countr_zero(attackBits);
            quiets.EmplaceBack(index, toIndex, piece, Piece::Invalid());
            attackBits &= (attackBits - 1);
        }
    }
};
