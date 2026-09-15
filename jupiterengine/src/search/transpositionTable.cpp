#include "transpositionTable.h"
#include <cassert>
#include <cstring>
#include "util/instrumenter.h"

#define ALWAYS_OVERWRITE false 
#define PREFER_DEPTH true

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
    const TTEntryPacked& entry = m_Table[index];
    if (entry.hash == key)
        return TTEntry(entry);
    return TTEntry::Invalid();
}

void TranspositionTable::Save(ZobristKey key, int32_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType)
{
    JUPITER_TRACE();

    std::size_t index = Index(key);
    const TTEntryPacked& oldEntry = m_Table[index];

    if (!oldEntry.IsValid()) {
        m_Occupancy++;
        m_Table[index] = TTEntryPacked(key, score, depth, bestMove, nodeType);
        return;
    }

#if ALWAYS_OVERWRITE
    m_Table[index] = PackedTableEntry(key, score, depth, bestMove, nodeType);
#elif PREFER_DEPTH 
    TTEntry oldEntryUnpacked(oldEntry);
    if (depth >= oldEntryUnpacked.depth)
        m_Table[index] = TTEntryPacked(key, score, depth, bestMove, nodeType);
#endif
}
