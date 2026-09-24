#include "transpositionTable.h"
#include <cassert>
#include <cstring>
#include "util/instrumenter.h"

TranspositionTable::TranspositionTable()
{
    JUPITER_TRACE();

    m_Table = new TTEntryPacked[TRANSPOSITION_TABLE_SIZE];
    std::memset(m_Table, 0, TRANSPOSITION_TABLE_SIZE * sizeof(TTEntryPacked));
}

TranspositionTable::~TranspositionTable()
{
    delete[] m_Table;
}

TTEntry TranspositionTable::Get(ZobristKey key)
{
    JUPITER_TRACE();

    std::size_t index = Index(key);
    TTEntryPacked& entry = m_Table[index];
    if (entry.IsValid() && entry.hash == key)
        return TTEntry(entry);
    return TTEntry::Invalid();
}

void TranspositionTable::Save(ZobristKey key, uint8_t halfMove, int32_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType)
{
    JUPITER_TRACE();

    std::size_t index = Index(key);
    const TTEntryPacked& oldEntry = m_Table[index];

    if (!oldEntry.IsValid()) {
        m_Occupancy++;
        m_Table[index] = TTEntryPacked(key, halfMove, score, depth, bestMove, nodeType);
    } else {
        TTEntry oldEntryUnpacked(oldEntry);
        uint8_t ageDifference = (halfMove - oldEntryUnpacked.age) & 15;
        int32_t oldScore = (static_cast<int32_t>(oldEntryUnpacked.depth) << 2) - static_cast<int32_t>(ageDifference);
        int32_t newScore = static_cast<int32_t>(depth) << 2;
        if (newScore >= oldScore)
            m_Table[index] = TTEntryPacked(key, halfMove, score, depth, bestMove, nodeType);
    }
}
