#pragma once

#include "board/bitboard.h"
#include "datastructure/buffer.h"
#include "util/instrumenter.h"
#include "util/rng.h"
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

const int8_t PAWN_ATTACKS[2][2] = { { 1, 1 }, { -1, 1 } }; // Subject to direction
const int8_t KNIGHT_ATTACKS[8][2] = { { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 }, { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } };
const int8_t BISHOP_ATTACKS[4][2] = { { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 } };
const int8_t ROOK_ATTACKS[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
const int8_t KING_ATTACKS[8][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 } };

template<Piece::Value piece> struct SliderData {
    static_assert(piece == Piece::ROOK || piece == Piece::BISHOP);

    // Rooks need up to 12 bits worth of slots, bishops need 9 
    // TODO: Compress the tableMemory to have the exact right amount of space.
    //       Overallocating most of these and most squares won't need all bits.
    //       This hurts cache locality as there is lots of padding but is much simpler.
    //       Still better to do it this way than allocate each square's table separately.
    SliderData()
        : tableMemory{new Bitboard[64 * (1ull << (piece == Piece::ROOK ? 12 : 9))]} {}

    Buffer<Bitboard, 64> masks{Buffer<Bitboard, 64>(64)};
    Buffer<Bitboard, 64> magics{Buffer<Bitboard, 64>(64)};
    Buffer<uint8_t, 64> shifts{Buffer<uint8_t, 64>(64)};
    Buffer<Bitboard *, 64> tables{Buffer<Bitboard *, 64>(64)};
    Bitboard (*tableMemory){nullptr};
};

class AttackTable {
public:
    AttackTable();
    bool SquareUnderAttack(const class BoardState& state, uint64_t bit, Color::Value color) const;
    Bitboard PawnAttacks(uint8_t index, Color::Value color) const;
    Bitboard KnightAttacks(uint8_t index) const;
    Bitboard BishopAttacks(uint8_t index, Bitboard occupancy) const;
    Bitboard RookAttacks(uint8_t index, Bitboard occupancy) const;
    Bitboard QueenAttacks(uint8_t index, Bitboard occupancy) const;
    Bitboard KingAttacks(uint8_t index) const;

private:
    void SerializeMagics(const fs::path& path);
    void DeserializeMagics(const fs::path& path);
    void GeneratePawnTables();
    void GenerateKnightTables();
    void GenerateKingTables();
    void GenerateSliderTables();
    Bitboard GenerateSliderMask(uint8_t index, const int8_t (& deltas)[4][2]);
    Bitboard GenerateSliderAttacks(uint8_t index, Bitboard occupancy, const int8_t (& deltas)[4][2]);
    template<Piece::Value piece> Bitboard FindMagic(uint8_t index, RomuQuadRandom& rng, uint64_t maxAttempts = 10'000'000)
    {
        JUPITER_TRACE();

        auto SparseRandom = [&rng]() {
            uint64_t random;
            do {
                random = rng.Generate() & rng.Generate() & rng.Generate();
            } while (std::popcount(random) < 6);
            return random;
        };

        Bitboard mask;
        if constexpr (piece == Piece::ROOK) mask = m_RookData.masks[index];
        else mask = m_BishopData.masks[index];
        uint8_t bitCount = std::popcount(mask); 

        Buffer<Bitboard, 1ull << 12> occupancies;
        Bitboard sub = mask;
        do {
            occupancies.PushBack(sub);
            sub = (sub - 1) & mask;
        } while (sub != mask);

        Buffer<Bitboard, 1ull << 12> usedAttacks(1ull << 12);
        for (uint64_t attempt = 0; attempt < maxAttempts; attempt++) {
            uint64_t magic = SparseRandom();

            std::fill(usedAttacks.begin(), usedAttacks.end(), 0);
            for (const auto occupancy : occupancies) {
                uint16_t magicIndex = static_cast<uint16_t>((occupancy * magic) >> (64 - bitCount));
                Bitboard attacks = [this, occupancy, index] {
                    if constexpr (piece == Piece::ROOK) return GenerateSliderAttacks(index, occupancy, ROOK_ATTACKS);
                    else return GenerateSliderAttacks(index, occupancy, BISHOP_ATTACKS);
                }();

                if (usedAttacks[magicIndex] == 0)
                    usedAttacks[magicIndex] = attacks;
                else if (usedAttacks[magicIndex] != attacks)
                    goto failed;
            }
            return magic;

            failed:
                continue;
        }

        return 0;
    }

private:
    Buffer<Bitboard, 64> m_KnightTables;
    Buffer<Bitboard, 64> m_KingTables;
    Buffer<Bitboard, 64> m_PawnTables[Color::MAX_ENUM];

    SliderData<Piece::BISHOP> m_BishopData;
    SliderData<Piece::ROOK> m_RookData;

    bool m_MagicsLoadedFromFile{false};
};
