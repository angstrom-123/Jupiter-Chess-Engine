#include "transpositionTable.h"
#include <cassert>
#include <cstring>
#include "util/instrumenter.h"

#define ALWAYS_OVERWRITE false 
#define PREFER_DEPTH true

TranspositionTable::TranspositionTable()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_Table = new PackedTableEntry[TRANSPOSITION_TABLE_SIZE];
    std::memset(m_Table, 0, TRANSPOSITION_TABLE_SIZE * sizeof(PackedTableEntry));
}

TranspositionTable::~TranspositionTable()
{
    delete[] m_Table;
}

TableEntry TranspositionTable::Get(ZobristKey key)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint64_t index = Index(key);
    const PackedTableEntry& entry = m_Table[index];
    if (entry.hash == key)
        return TableEntry(entry);
    return TableEntry::Invalid();
}

void TranspositionTable::Save(const BoardState& state, int32_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint64_t index = Index(state.zobristKey);
    const PackedTableEntry& oldEntry = m_Table[index];

    if (!oldEntry.IsValid()) {
        m_Occupancy++;
        m_Table[index] = PackedTableEntry(state.zobristKey, score, depth, bestMove, nodeType);
        return;
    }

#if ALWAYS_OVERWRITE
    m_Table[index] = PackedTableEntry(state.zobristKey, score, depth, bestMove, nodeType);
#elif PREFER_DEPTH 
    TableEntry oldEntryUnpacked(oldEntry);
    if (depth >= oldEntryUnpacked.depth)
        m_Table[index] = PackedTableEntry(state.zobristKey, score, depth, bestMove, nodeType);
#endif
}

uint64_t TranspositionTable::Index(ZobristKey key) const 
{
    JUPITER_TRACE();

    return key & (TRANSPOSITION_TABLE_SIZE - 1);
}
