#pragma once

#include <cstdint>

class DistanceTable {
public:
    DistanceTable();
    uint8_t Manhattan(uint8_t fromIndex, uint8_t toIndex) const;
    uint8_t ManhattanFromCenter(uint8_t index) const;

private:
    uint8_t ManhattanDistance(uint8_t fromIndex, uint8_t toIndex) const;

private:
    uint8_t m_Manhattan[64][64];
    uint8_t m_FromCenter[64];
};
