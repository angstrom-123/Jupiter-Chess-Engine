#pragma once 

#include "board/bitboard.h"

struct Direction {
    typedef enum {
        LEFT,
        RIGHT,
        MAX_ENUM
    } Value;

    static constexpr Buffer<Direction::Value, Direction::MAX_ENUM> values{Direction::LEFT, Direction::RIGHT};
};

class MaskTable {
public:
    MaskTable();
    Bitboard ConnectedPawns(Color::Value color, uint8_t nPawns, Direction::Value direction, uint8_t index) const { return m_ConnectedPawns[color][nPawns - 1][direction][index]; }
    Bitboard NeighbouringFiles(uint8_t file) const { return m_NeighbouringFiles[file]; }
    Bitboard PassedPawnBlockers(Color::Value color, uint8_t index) const { return m_PassedPawnBlockers[color][index]; }

private:
    void ComputeConnectedPawns();
    void ComputeNeighbouringFiles();
    void ComputePassedPawnBlockers();

private:
    Bitboard m_ConnectedPawns[Color::MAX_ENUM][6][Direction::MAX_ENUM][64]{};
    Bitboard m_NeighbouringFiles[8]{};
    Bitboard m_PassedPawnBlockers[Color::MAX_ENUM][64]{};
};
