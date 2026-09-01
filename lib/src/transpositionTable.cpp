#include "transpositionTable.h"
#include <cstring>
#include "instrumenter.h"

#define ALWAYS_OVERWRITE false 
#define PREFER_DEPTH true

TranspositionTable::TranspositionTable()
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    m_Table = new TableEntry[TRANSPOSITION_TABLE_SIZE];
    std::memset(m_Table, 0, TRANSPOSITION_TABLE_SIZE * sizeof(TableEntry));
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
    const TableEntry& entry = m_Table[index];
    if (entry.hash == key)
        return entry;
    return TableEntry::Invalid();
}

void TranspositionTable::Save(const BoardState& state, int64_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint64_t index = Index(state.zobristKey);
    const TableEntry& oldEntry = m_Table[index];

    if (!oldEntry.IsValid()) {
        m_Occupancy++;
        m_Table[index] = TableEntry(state.zobristKey, score, bestMove, depth, nodeType);
        return;
    }

#if ALWAYS_OVERWRITE
    m_Table[index] = TableEntry(state.zobristKey, score, bestMove, depth, nodeType);
#elif PREFER_DEPTH 
    if (depth >= oldEntry.depth)
        m_Table[index] = TableEntry(state.zobristKey, score, bestMove, depth, nodeType);
#endif
}

uint64_t TranspositionTable::Index(ZobristKey key) const 
{
    JUPITER_TRACE();

    return key & (TRANSPOSITION_TABLE_SIZE - 1);
}
