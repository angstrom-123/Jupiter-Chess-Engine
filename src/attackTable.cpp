#include "attackTable.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <immintrin.h>
#include <iostream>
#include <iterator>
#include <popcntintrin.h>
#include <array>

const std::array<int8_t[2], 2> PAWN_ATTACKS = {{ { 1, 1 }, { -1, 1 } }}; // Subject to direction
const std::array<int8_t[2], 8> KNIGHT_ATTACKS = {{ { 1, 2 }, { 2, 1 }, { 2, -1 }, { 1, -2 }, { -1, -2 }, { -2, -1 }, { -2, 1 }, { -1, 2 } }};
const std::array<int8_t[2], 4> BISHOP_ATTACKS = {{ { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 } }};
const std::array<int8_t[2], 4> ROOK_ATTACKS = {{ { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } }};
const std::array<int8_t[2], 8> KING_ATTACKS = {{ { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 } }};

const fs::path MAGIC_FILE_PATH = fs::path("engine") / "magics.bin";

AttackTable::AttackTable()
{
    GeneratePawnTables();
    GenerateKnightTables();
    GenerateKingTables();
    GenerateSliderTables();
}

Bitboard AttackTable::GetAttacks(uint8_t index, Piece::Value piece, Color::Value color, Bitboard occupancy) const
{
    switch (piece) {
        case Piece::PAWN:
            return m_PawnTables[color][index];
        case Piece::KNIGHT:
            return m_KnightTables[index];
        case Piece::BISHOP:
        {
            Bitboard blockers = occupancy & m_BishopData.masks[index];
            uint16_t tableIndex = static_cast<uint16_t>((blockers * m_BishopData.magics[index]) >> m_BishopData.shifts[index]);
            return m_BishopData.tables[index][tableIndex];
        }
        case Piece::ROOK:
        {
            Bitboard blockers = occupancy & m_RookData.masks[index];
            uint16_t tableIndex = static_cast<uint16_t>((blockers * m_RookData.magics[index]) >> m_RookData.shifts[index]);
            return m_RookData.tables[index][tableIndex];
        }
        case Piece::QUEEN:
        {
            Bitboard blockers = occupancy & m_BishopData.masks[index];
            uint16_t tableIndex = static_cast<uint16_t>((blockers * m_BishopData.magics[index]) >> m_BishopData.shifts[index]);
            Bitboard bishopAttacks = m_BishopData.tables[index][tableIndex];

            blockers = occupancy & m_RookData.masks[index];
            tableIndex = static_cast<uint16_t>((blockers * m_RookData.magics[index]) >> m_RookData.shifts[index]);
            Bitboard rookAttacks = m_RookData.tables[index][tableIndex];

            return bishopAttacks | rookAttacks;
        }
        case Piece::KING:
            return m_KingTables[index];
        default:
            return UINT64_MAX;
    }
}

void AttackTable::SerializeMagics(const fs::path& path)
{
    std::ofstream file(path, std::ios::out | std::ios::binary);
    std::ostreambuf_iterator<char> it(file);
    std::copy(reinterpret_cast<const char *>(m_RookData.magics.Data()), reinterpret_cast<const char *>(m_RookData.magics.Data() + 64), it);
    std::copy(reinterpret_cast<const char *>(m_BishopData.magics.Data()), reinterpret_cast<const char *>(m_BishopData.magics.Data() + 64), it);
}

void AttackTable::DeserializeMagics(const fs::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    file.read(reinterpret_cast<char *>(m_RookData.magics.Data()), 64 * sizeof(uint64_t));
    file.read(reinterpret_cast<char *>(m_BishopData.magics.Data()), 64 * sizeof(uint64_t));
}

void AttackTable::GeneratePawnTables()
{
    for (uint8_t i = 0; i < 64; i++) {
        Bitboard mask = 0;
        int8_t x = i % 8;
        int8_t y = i / 8;
        for (const auto delta : PAWN_ATTACKS) {
            int8_t xx = x - delta[0];
            int8_t yy = y - delta[1];
            if (xx >= 0 && xx < 8 && yy >= 0 && yy < 8)
                mask |= (1ul << ToIndex(xx, yy));
        }
        m_PawnTables[Color::WHITE][i] = mask;
    }

    for (uint8_t i = 0; i < 64; i++) {
        Bitboard mask = 0;
        int8_t x = i % 8;
        int8_t y = i / 8;
        for (const auto delta : PAWN_ATTACKS) {
            int8_t xx = x + delta[0];
            int8_t yy = y + delta[1];
            if (xx >= 0 && xx < 8 && yy >= 0 && yy < 8)
                mask |= (1ul << ToIndex(xx, yy));
        }
        m_PawnTables[Color::BLACK][i] = mask;
    }
}

void AttackTable::GenerateKnightTables()
{
    for (uint8_t i = 0; i < 64; i++) {
        Bitboard mask = 0;
        int8_t x = i % 8;
        int8_t y = i / 8;
        for (const auto delta : KNIGHT_ATTACKS) {
            int8_t xx = x + delta[0];
            int8_t yy = y + delta[1];
            if (xx >= 0 && xx < 8 && yy >= 0 && yy < 8)
                mask |= (1ul << ToIndex(xx, yy));
        }
        m_KnightTables[i] = mask;
    }
}

void AttackTable::GenerateKingTables()
{
    for (uint8_t i = 0; i < 64; i++) {
        Bitboard mask = 0;
        int8_t x = i % 8;
        int8_t y = i / 8;
        for (const auto delta : KING_ATTACKS) {
            int8_t xx = x + delta[0];
            int8_t yy = y + delta[1];
            if (xx >= 0 && xx < 8 && yy >= 0 && yy < 8)
                mask |= (1ul << ToIndex(xx, yy));
        }
        m_KingTables[i] = mask;
    }
}

void AttackTable::GenerateSliderTables()
{
    if (fs::exists(MAGIC_FILE_PATH)) {
        DeserializeMagics(MAGIC_FILE_PATH);
        m_MagicsLoadedFromFile = true;
    }

    XorShift64 rng(123456789);
    for (uint8_t i = 0; i < 64; i++) {
        // Rook
        {
            m_RookData.masks[i] = GenerateSliderMask(i, ROOK_ATTACKS);
            m_RookData.shifts[i] = 64 - _mm_popcnt_u64(m_RookData.masks[i]);
            if (!m_MagicsLoadedFromFile)
                m_RookData.magics[i] = FindMagic(i, rng, Piece::ROOK);

            uint64_t rookSize = 1ul << (64 - m_RookData.shifts[i]);
            m_RookData.tables[i].resize(rookSize);
            Bitboard rookSub = m_RookData.masks[i];
            do {
                uint16_t index = static_cast<uint16_t>((rookSub * m_RookData.magics[i]) >> m_RookData.shifts[i]);
                m_RookData.tables[i][index] = GenerateSliderAttacks(i, rookSub, ROOK_ATTACKS);
                rookSub = (rookSub - 1) & m_RookData.masks[i];
            } while (rookSub != m_RookData.masks[i]);
        }

        // Bishop
        {
            m_BishopData.masks[i] = GenerateSliderMask(i, BISHOP_ATTACKS);
            m_BishopData.shifts[i] = 64 - _mm_popcnt_u64(m_BishopData.masks[i]);
            if (!m_MagicsLoadedFromFile)
                m_BishopData.magics[i] = FindMagic(i, rng, Piece::BISHOP);

            uint64_t bishopSize = 1ul << (64 - m_BishopData.shifts[i]);
            m_BishopData.tables[i].resize(bishopSize);
            Bitboard bishopSub = m_BishopData.masks[i];
            do {
                uint16_t index = static_cast<uint16_t>((bishopSub * m_BishopData.magics[i]) >> m_BishopData.shifts[i]);
                m_BishopData.tables[i][index] = GenerateSliderAttacks(i, bishopSub, BISHOP_ATTACKS);
                bishopSub = (bishopSub - 1) & m_BishopData.masks[i];
            } while (bishopSub != m_BishopData.masks[i]);
        }
    }

    if (!m_MagicsLoadedFromFile) {
        SerializeMagics(MAGIC_FILE_PATH);
        m_MagicsLoadedFromFile = true;
    }
}

Bitboard AttackTable::GenerateSliderMask(uint8_t index, const std::array<int8_t[2], 4>& deltas)
{
    Bitboard mask = 0;
    int8_t x = index % 8;
    int8_t y = index / 8;
    for (const auto delta : deltas) {
        // NOTE: Stopping one square shy of the edge since a piece there blocks no squares behind it
        for (int8_t distance = 1; distance < 7; distance++) {
            int8_t xx = x + delta[0] * distance;
            int8_t yy = y + delta[1] * distance;

            if (xx < 0 || xx > 7 || yy < 0 || yy > 7)
                break;

            // Exclude blockers at the outer edge of the board since they have no squares behind them
            int8_t xxNext = xx + delta[0];
            int8_t yyNext = yy + delta[1];
            if (xxNext < 0 || xxNext > 7 || yyNext < 0 || yyNext > 7)
                break;

            mask |= (1ul << ToIndex(xx, yy));
        }
    }
    return mask;
}

Bitboard AttackTable::GenerateSliderAttacks(uint8_t index, Bitboard occupancy, const std::array<int8_t[2], 4>& deltas)
{
    Bitboard mask = 0;
    int8_t x = index % 8;
    int8_t y = index / 8;
    for (const auto delta : deltas) {
        for (int8_t distance = 1; distance < 8; distance++) {
            int8_t xx = x + delta[0] * distance;
            int8_t yy = y + delta[1] * distance;
            if (xx < 0 || xx > 7 || yy < 0 || yy > 7)
                break;

            uint64_t bit = 1ul << ToIndex(xx, yy);
            mask |= bit;

            if (occupancy & bit)
                break;
        }
    }
    return mask;
}

Bitboard AttackTable::FindMagic(uint8_t index, XorShift64& rng, Piece::Value piece, uint64_t maxAttempts)
{
    const SliderData& data = (piece == Piece::ROOK) ? m_RookData : m_BishopData;

    auto SparseRandom = [&rng]() {
        uint64_t random;
        do {
            random = rng.Next() & rng.Next() & rng.Next();
        } while (_mm_popcnt_u64(random) < 6);
        return random;
    };

    Bitboard mask = data.masks[index];
    uint8_t bitCount = _mm_popcnt_u64(mask); 

    Buffer<Bitboard, 1ul << 12> occupancies;
    Bitboard sub = mask;
    do {
        occupancies.PushBack(sub);
        sub = (sub - 1) & mask;
    } while (sub != mask);

    Buffer<Bitboard, 1ul << 12> usedAttacks(1ul << 12);
    for (uint64_t attempt = 0; attempt < maxAttempts; attempt++) {
        uint64_t magic = SparseRandom();

        std::fill(usedAttacks.begin(), usedAttacks.end(), 0);
        for (const auto occupancy : occupancies) {
            uint16_t magicIndex = static_cast<uint16_t>((occupancy * magic) >> (64 - bitCount));
            Bitboard attacks = (piece == Piece::ROOK) 
                    ? GenerateSliderAttacks(index, occupancy, ROOK_ATTACKS)
                    : GenerateSliderAttacks(index, occupancy, BISHOP_ATTACKS);

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
