#include "transpositionTable.h"
#include <cstring>
#include "instrumenter.h"

#define ALWAYS_OVERWRITE false 
#define PREFER_DEPTH true

TableEntry PackedTableEntry::Unpack(uint64_t index) const
{
    TableEntry res;

    uint64_t moveBits = packed & ((1ul << 19) - 1);
    uint8_t toFile   = (moveBits & 0b000000000000000111);
    uint8_t toRow    = (moveBits & 0b000000000000111000) >> 3;
    uint8_t fromFile = (moveBits & 0b000000000111000000) >> 6;
    uint8_t fromRow  = (moveBits & 0b000000111000000000) >> 9;
    uint8_t promote  = (moveBits & 0b000111000000000000) >> 12;
    uint8_t piece    = (moveBits & 0b111000000000000000) >> 15;

    res.bestMove.from = ToIndex(fromFile, fromRow);
    res.bestMove.to = ToIndex(toFile, toRow);
    res.bestMove.piece = static_cast<Piece::Value>(piece);
    res.bestMove.promote = static_cast<Piece::Value>(promote);
    res.depth = (packed >> 18) & 0b11111;
    res.nodeType = static_cast<NodeType::Value>((packed >> 26) & 0b11);
    res.hash = (packed & ~((1ul << TRANSPOSITION_TABLE_KEY_BITS) - 1)) | index;
    res.score = score;
    return res;
}

// 64 (2 nodetype, 5 depth, 18 bestmove) + 39 key
PackedTableEntry::PackedTableEntry(uint64_t hash, int64_t score, Move bestMove, uint8_t depth, NodeType::Value nodeType)
{
    uint64_t packedMove = 0;
    packedMove |= static_cast<uint64_t>(bestMove.piece) << 15;
    packedMove |= static_cast<uint64_t>(bestMove.promote) << 12;
    packedMove |= static_cast<uint64_t>(bestMove.from / 8) << 9;
    packedMove |= static_cast<uint64_t>(bestMove.from & 7) << 6;
    packedMove |= static_cast<uint64_t>(bestMove.to / 8) << 3;
    packedMove |= static_cast<uint64_t>(bestMove.to & 7);

    packed |= hash & ~((1ul << TRANSPOSITION_TABLE_KEY_BITS) - 1); // Mask off bottom (index) bits
    packed |= static_cast<uint64_t>(nodeType) << 23;
    packed |= static_cast<uint64_t>(depth) << 18;
    packed |= packedMove;
    this->score = score;
}

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
    const PackedTableEntry& packedEntry = m_Table[index];
    if (!packedEntry.IsValid())
        return TableEntry::Invalid();
    
    const TableEntry entry = packedEntry.Unpack(index);
    if (entry.hash == key)
        return entry;

    return TableEntry::Invalid();
}

void TranspositionTable::Save(const BoardState& state, int64_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType)
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint64_t index = Index(state.zobristKey);
    const PackedTableEntry& oldEntry = m_Table[index];

    if (!oldEntry.IsValid()) {
        m_Occupancy++;
        m_Table[index] = PackedTableEntry(state.zobristKey, score, bestMove, depth, nodeType);
        return;
    }

#if ALWAYS_OVERWRITE
    m_Table[index] = PackedTableEntry(state.zobristKey, score, bestMove, depth, nodeType);
#elif PREFER_DEPTH 
    if (depth >= oldEntry.Unpack(index).depth)
        m_Table[index] = PackedTableEntry(state.zobristKey, score, bestMove, depth, nodeType);
#endif
}

uint64_t TranspositionTable::Index(ZobristKey key) const 
{
    JUPITER_TRACE();

    return key & (TRANSPOSITION_TABLE_SIZE - 1);
}
