#include "maskTable.h"
#include "board/bitboard.h"
#include "core.h"
#include <cstring>

static const Bitboard FILE_MASK = 0b00000001'00000001'00000001'00000001'00000001'00000001'00000001'00000001;

MaskTable::MaskTable()
{
    ComputeConnectedPawns();
    ComputeNeighbouringFiles();
    ComputePassedPawnBlockers();
}

void MaskTable::ComputeConnectedPawns()
{
    // Fill with sentinel value
    std::memset(m_ConnectedPawns, 1, sizeof(m_ConnectedPawns));

    // Construct masks for a pawn of each color at each index for each direction and distance 
    // Then binary search can be done in eval to find the maximum chain length (1-6)
    // The sentinels (all 1s) ensure that search condition fails - no need to check validity
    const int8_t offsets[Color::MAX_ENUM][2][2] = { { { 1, -1 }, { -1, -1 } }, { { 1, 1 }, { -1, 1 } } };
    for (const Color::Value color : Color::values) {
        for (uint8_t i = 0; i < 64; i++) {
            uint8_t rank = i % 8;
            uint8_t file = i & 7;

            for (const Direction::Value direction : Direction::values) {
                Bitboard mask = 0;

                // Maximum chain length is 6
                for (uint8_t distance = 1; distance < 7; distance++) {
                    int8_t x = offsets[color][direction][0] * distance + file;
                    int8_t y = offsets[color][direction][1] * distance + rank;

                    if (x < 0 || x > 7 || y < 0 || y > 7)
                        break;

                    uint8_t index = 8 * y + x;
                    mask |= (1ull << index);

                    m_ConnectedPawns[color][distance - 1][direction][i] = mask;
                }
            }
        }
    }
}

void MaskTable::ComputeNeighbouringFiles()
{
    // Just the file to the left and right
    for (uint8_t file = 0; file < 8; file++) {
        Bitboard mask = 0;
        if (file > 0) mask |= (FILE_MASK << (file - 1));
        if (file < 7) mask |= (FILE_MASK << (file + 1));

        m_NeighbouringFiles[file] = mask;
    }
}

void MaskTable::ComputePassedPawnBlockers()
{
    std::memset(m_PassedPawnBlockers, 0, sizeof(m_PassedPawnBlockers));
    for (const Color::Value color : Color::values) {
        for (uint8_t i = 8; i < 56; i++) {
            uint8_t rank = i / 8;
            uint8_t file = i & 7;

            // Mask for the current and adjacent files
            Bitboard mask = FILE_MASK << file;
            if (file > 0) mask |= (FILE_MASK << (file - 1));
            if (file < 7) mask |= (FILE_MASK << (file + 1));

            // Push mask up to ignore pawns behind or directly beside this one
            if (color == Color::WHITE)
                mask >>= (8 - rank) * 8;
            else 
                mask <<= (rank + 1) * 8;

            m_PassedPawnBlockers[color][i] = mask;
        }
    }
}
