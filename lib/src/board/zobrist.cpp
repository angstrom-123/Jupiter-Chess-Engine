#include "zobrist.h"
#include "core.h"
#include "util/rng.h"
#include "util/instrumenter.h"
#include "boardState.h"

#include <bit>

Zobrist::Zobrist()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint64_t seed[4] = { 1, 2, 3, 4 };
    RomuQuadRandom rng(seed);
    rng.Warm();

    for (uint64_t i = 0; i < ZOBRIST_NUMBER_COUNT; i++)
        m_Randoms[i] = rng.Generate();
}

ZobristKey Zobrist::ComputeKey(const BoardState& state) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    ZobristKey key = 0;
    uint64_t offset = 0;

    // Pieces
    for (const Color::Value color : { Color::WHITE, Color::BLACK }) {
        for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
            Bitboard occupancy = state.pieces.OccupancyMask(color, piece);
            while (occupancy) {
                uint8_t index = std::countr_zero(occupancy);
                key ^= m_Randoms[offset + index];
                occupancy &= (occupancy - 1);
            }
            offset += 64;
        }
    }

    // Castling rights
    if (state.rights & CastlingRight::KINGSIDE_WHITE)
        key ^= m_Randoms[offset];
    offset++;

    if (state.rights & CastlingRight::QUEENSIDE_WHITE)
        key ^= m_Randoms[offset];
    offset++;

    if (state.rights & CastlingRight::KINGSIDE_BLACK)
        key ^= m_Randoms[offset];
    offset++;

    if (state.rights & CastlingRight::QUEENSIDE_BLACK)
        key ^= m_Randoms[offset];
    offset++;

    // En Passant File
    if (state.enPassantIndex != UINT8_MAX) {
        // Check if any of our pawns can en passant the opponent pawn that just double pushed
        // This means that two identical positions (except for the en passant square) will hash to 
        // the same value as long as there is no pawn to capture en passant.
        // This check only accounts for pseudo-legal en passant captures but is better than nothing.
        uint8_t file = state.enPassantIndex & 7;
        uint8_t square = 8 * (state.turn == Color::WHITE ? 3 : 4) + file;
        uint64_t adjacentMask = 0;
        if (file > 0) adjacentMask |= (1ul << (square - 1));
        if (file < 7) adjacentMask |= (1ul << (square + 1));
        if (adjacentMask & state.pieces.OccupancyMask(state.turn, Piece::PAWN))
            key ^= m_Randoms[offset + file];
    }
    offset += 8;

    // Turn to move
    if (state.turn == Color::BLACK)
        key ^= m_Randoms[offset];
    offset++;

    return key;
}

uint64_t Zobrist::ValueForPiece(Color::Value color, Piece::Value piece, uint8_t index) const
{
    uint64_t colorOffset = color * (Piece::MAX_ENUM * 64);
    uint64_t pieceOffset = piece * 64;
    uint64_t indexOffset = index;
    return m_Randoms[colorOffset + pieceOffset + indexOffset];
}

uint64_t Zobrist::ValueForRights(CastlingRights rights) const
{
    constexpr uint64_t rightsOffset = 2 * (Piece::MAX_ENUM * 64);
    uint64_t value = 0;
    if (rights & CastlingRight::KINGSIDE_WHITE)
        value ^= m_Randoms[rightsOffset];
    if (rights & CastlingRight::QUEENSIDE_WHITE)
        value ^= m_Randoms[rightsOffset + 1];
    if (rights & CastlingRight::KINGSIDE_BLACK)
        value ^= m_Randoms[rightsOffset + 2];
    if (rights & CastlingRight::QUEENSIDE_BLACK)
        value ^= m_Randoms[rightsOffset + 3];
    return value;
}

uint64_t Zobrist::ValueForEnPassant(uint8_t enPassantIndex) const
{
    constexpr uint64_t enPassantOffset = 2 * (Piece::MAX_ENUM * 64) + 4;

    if (enPassantIndex == UINT8_MAX)
        return 0;

    uint8_t file = enPassantIndex & 7;
    return m_Randoms[enPassantOffset + file];
}

uint64_t Zobrist::ValueForTurn(Color::Value turn) const
{
    if (turn == Color::BLACK)
        return m_Randoms[ZOBRIST_NUMBER_COUNT - 1];

    return 0;
}
