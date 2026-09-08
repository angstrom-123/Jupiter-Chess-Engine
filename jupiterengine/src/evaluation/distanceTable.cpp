#include "distanceTable.h"

DistanceTable::DistanceTable()
{
    // Manhattan distance lookup between any 2 indices
    for (uint8_t i = 0; i < 64; i++) {
        for (uint8_t j = 0; j < 64; j++) {
            m_Manhattan[i][j] = ManhattanDistance(i, j);
        }
    }

    // Average manhattan distance from the center squares
    for (uint8_t i = 0; i < 64; i++) {
        uint8_t tl = ManhattanDistance(i, 27);
        uint8_t tr = ManhattanDistance(i, 28);
        uint8_t bl = ManhattanDistance(i, 35);
        uint8_t br = ManhattanDistance(i, 36);
        m_FromCenter[i] = (tl + tr + bl + br) / 4;
    }
}

uint8_t DistanceTable::Manhattan(uint8_t fromIndex, uint8_t toIndex) const 
{
    return m_Manhattan[fromIndex][toIndex];
}

uint8_t DistanceTable::ManhattanFromCenter(uint8_t index) const 
{
    return m_FromCenter[index];
}

uint8_t DistanceTable::ManhattanDistance(uint8_t fromIndex, uint8_t toIndex) const 
{
    uint8_t fromCol = fromIndex & 7;
    uint8_t fromRow = fromIndex / 8;

    uint8_t toCol = toIndex & 7;
    uint8_t toRow = toIndex / 8;

    uint8_t deltaRow = (fromRow >= toRow) ? fromRow - toRow : toRow - fromRow;
    uint8_t deltaCol = (fromCol >= toCol) ? fromCol - toCol : toCol - fromCol;

    return deltaRow + deltaCol;
}
