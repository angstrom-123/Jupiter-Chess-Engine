#include "pawnTable.h"

PawnTable::PawnTable()
{
    m_Table = new PTEntry[PAWN_TABLE_SIZE];
}

PawnTable::~PawnTable()
{
    delete[] m_Table;
}

PTEntry PawnTable::Get(ZobristKey pawnKey) const
{
    std::size_t index = Index(pawnKey);
    PTEntry& entry = m_Table[index];
    if (entry.hash == pawnKey)
        return entry;

    return PTEntry::Invalid();
}

void PawnTable::Save(ZobristKey pawnKey, int32_t score, PawnStructure& structure)
{
    // Do not save positions with 0 pawns
    if (pawnKey == 0)
        return;

    std::size_t index = Index(pawnKey);
    const PTEntry& oldEntry = m_Table[index];
    if (!oldEntry.IsValid())
        m_Occupancy++;
    m_Table[index] = PTEntry(pawnKey, score, structure);
}
