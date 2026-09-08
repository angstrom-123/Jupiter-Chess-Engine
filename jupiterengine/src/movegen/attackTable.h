#pragma once

#include "board/bitboard.h"
#include "datastructure/buffer.h"
#include "util/rng.h"
#include <cstdint>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct SliderData {
    SliderData()
        : masks{Buffer<Bitboard, 64>(64)}, magics{Buffer<Bitboard, 64>(64)},
            shifts{Buffer<uint16_t, 64>(64)}, tables{Buffer<std::vector<Bitboard>, 64>(64)} {}

    Buffer<Bitboard, 64> masks;
    Buffer<Bitboard, 64> magics;
    Buffer<uint16_t, 64> shifts;
    Buffer<std::vector<Bitboard>, 64> tables;
};

class AttackTable {
public:
    AttackTable();
    bool SquareUnderAttack(const class BoardState& state, uint64_t bit, Color::Value color) const;
    Bitboard GetPawnAttacks(uint8_t index, Color::Value color) const;
    Bitboard GetKnightAttacks(uint8_t index) const;
    Bitboard GetBishopAttacks(uint8_t index, Bitboard occupancy) const;
    Bitboard GetRookAttacks(uint8_t index, Bitboard occupancy) const;
    Bitboard GetQueenAttacks(uint8_t index, Bitboard occupancy) const;
    Bitboard GetKingAttacks(uint8_t index) const;

private:
    void SerializeMagics(const fs::path& path);
    void DeserializeMagics(const fs::path& path);
    void GeneratePawnTables();
    void GenerateKnightTables();
    void GenerateKingTables();
    void GenerateSliderTables();
    [[nodiscard]] Bitboard GenerateSliderMask(uint8_t index, const std::array<int8_t[2], 4>& deltas);
    [[nodiscard]] Bitboard GenerateSliderAttacks(uint8_t index, Bitboard occupancy, const std::array<int8_t[2], 4>& deltas);
    [[nodiscard]] Bitboard FindMagic(uint8_t index, RomuQuadRandom& rng, Piece::Value piece, uint64_t maxAttempts = 10000000);

private:
    Buffer<Bitboard, 64> m_KnightTables;
    Buffer<Bitboard, 64> m_KingTables;
    Buffer<Bitboard, 64> m_PawnTables[Color::MAX_ENUM];

    SliderData m_BishopData;
    SliderData m_RookData;

    bool m_MagicsLoadedFromFile{false};
};
