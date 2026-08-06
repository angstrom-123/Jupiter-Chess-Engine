#include "movegen.h"
#include <immintrin.h>

namespace movegen {
    void FindAttacks(const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        attackBuffer.Clear();

        // Pawns
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::PAWN);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindPawnAttacks(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Knights
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KNIGHT);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindKnightAttacks(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Bishops
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::BISHOP);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindBishopAttacks(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Rooks
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::ROOK);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindRookAttacks(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Queens
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::QUEEN);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindQueenAttacks(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Kings
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KING);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindKingAttacks(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
                occupancy &= (occupancy - 1);
            }
        }
    }

    void FindQuiets(const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        quietBuffer.Clear();

        // Pawns
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::PAWN);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindPawnQuiets(index, std::forward<const BoardState>(state), quietBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Knights
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KNIGHT);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindKnightQuiets(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Bishops
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::BISHOP);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindBishopQuiets(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Rooks
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::ROOK);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindRookQuiets(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Queens
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::QUEEN);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindQueenQuiets(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
                occupancy &= (occupancy - 1);
            }
        }

        // Kings
        {
            Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KING);
            while (occupancy) {
                uint8_t index = _tzcnt_u64(occupancy);
                FindKingQuiets(index, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
                occupancy &= (occupancy - 1);
            }
        }
    }

    void FindPawnAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back

        uint64_t enPassantBit = (state.enPassantIndex != UINT8_MAX) ? 1ul << state.enPassantIndex : 0;
        Bitboard attacks = table.GetAttacks(index, Piece::PAWN, state.turn, state.pieces.OccupancyMask());
        attacks &= ~state.pieces.OccupancyMask(state.turn);
        attacks &= (state.pieces.OccupancyMask(Color::Opposite(state.turn)) | enPassantBit);
        
        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            uint64_t toBit = 1ul << toIndex;
            if (toBit & backRankMask) {
                // Promotion
                attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
                attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
                attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
                attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
            } else {
                attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            }
            attacks &= (attacks - 1);
        }
    }

    void FindPawnQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
    {
        const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back
        const uint64_t homeSquareMask = (state.turn == Color::WHITE) ? 0xFF000000000000 : 0x000000000000FF00;

        // In all cases, need to check if the move is a promotion

        int8_t delta = (state.turn == Color::WHITE) ? -8 : 8;
        uint8_t toIndex = index + delta;
        uint64_t toBit = 1ul << toIndex;
        if (!state.pieces.Has(toIndex)) {
            if (toBit & backRankMask) {
                // Promotion
                quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
                quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
                quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
                quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
            } else {
                quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
                toIndex += delta;
                if (((1ul << index) & homeSquareMask) && !state.pieces.Has(toIndex))
                    quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            }
        }
    }

    void FindKnightAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        Bitboard attacks = table.GetAttacks(index, Piece::KNIGHT, state.turn, 0);
        attacks &= ~state.pieces.OccupancyMask(state.turn);
        attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            attackBuffer.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    void FindKnightQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        Bitboard attacks = table.GetAttacks(index, Piece::KNIGHT, state.turn, 0);
        attacks &= ~state.pieces.OccupancyMask();

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            quietBuffer.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    void FindKingAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        Bitboard attacks = table.GetAttacks(index, Piece::KING, state.turn, 0);
        attacks &= ~state.pieces.OccupancyMask(state.turn);
        attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            attackBuffer.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    void FindKingQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        // Attacks
        {
            Bitboard attacks = table.GetAttacks(index, Piece::KING, state.turn, 0);
            attacks &= ~state.pieces.OccupancyMask();

            while (attacks) {
                uint8_t toIndex = _tzcnt_u64(attacks);
                quietBuffer.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
                attacks &= (attacks - 1);
            }
        }

        // Castling
        {
            if ((state.rights & CastlingRight::Kingside(state.turn)) && !state.pieces.Has(index + 1) && !state.pieces.Has(index + 2))
                quietBuffer.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

            if ((state.rights & CastlingRight::Queenside(state.turn)) && !state.pieces.Has(index - 1) && !state.pieces.Has(index - 2))
                quietBuffer.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
        }
    }

    void FindBishopAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        FindSliderAttacks(index, Piece::BISHOP, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
    }

    void FindBishopQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        FindSliderQuiets(index, Piece::BISHOP, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
    }

    void FindRookAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        FindSliderAttacks(index, Piece::ROOK, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
    }

    void FindRookQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        FindSliderQuiets(index, Piece::ROOK, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
    }

    void FindQueenAttacks(uint8_t index, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        FindSliderAttacks(index, Piece::QUEEN, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), attackBuffer);
    }

    void FindQueenQuiets(uint8_t index, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        FindSliderQuiets(index, Piece::QUEEN, std::forward<const BoardState>(state), std::forward<const AttackTable>(table), quietBuffer);
    }

    void FindSliderAttacks(uint8_t index, Piece::Value piece, const BoardState& state, const AttackTable& table, AttackMoveBuffer& attackBuffer)
    {
        Bitboard attacks = table.GetAttacks(index, piece, state.turn, state.pieces.OccupancyMask());
        attacks &= ~state.pieces.OccupancyMask(state.turn);
        attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            attackBuffer.EmplaceBack(index, toIndex, piece, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    void FindSliderQuiets(uint8_t index, Piece::Value piece, const BoardState& state, const AttackTable& table, QuietMoveBuffer& quietBuffer)
    {
        Bitboard attacks = table.GetAttacks(index, piece, state.turn, state.pieces.OccupancyMask());
        attacks &= ~state.pieces.OccupancyMask();

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            quietBuffer.EmplaceBack(index, toIndex, piece, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }
}
