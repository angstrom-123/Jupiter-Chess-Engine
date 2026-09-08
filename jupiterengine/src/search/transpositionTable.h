#pragma once

#include <cstdint>
#include "movegen/move.h"
#include "board/boardState.h"
#include "board/zobrist.h"

constexpr uint64_t TRANSPOSITION_TABLE_KEY_BITS = 25;
constexpr std::size_t TRANSPOSITION_TABLE_SIZE = 1ull << TRANSPOSITION_TABLE_KEY_BITS;

// TODO: Lock free table and multithreading

struct NodeType {
    typedef enum : uint8_t {
        UPPER_BOUND = 0,    // alpha cutoff
        LOWER_BOUND = 1,    // beta cutoff
        EXACT = 2           // no cutoff
    } Value;

    static const char *Show(Value value) 
    {
        switch (value) {
            case UPPER_BOUND: return "Upper Bound";
            case LOWER_BOUND: return "Lower Bound";
            case EXACT: return "Exact";
        }
    }
};

struct PackedTableEntry {
    ZobristKey hash{0};
    int32_t score{0};
    uint32_t payload{0};

    PackedTableEntry() = default;
    PackedTableEntry(ZobristKey hash, int32_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType)
        : hash{hash}, score{score}
    {
        // 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 
        // ------- depth---------- ----age---- --promo- --piece- -----from-------  -------to-------  type

        payload |= static_cast<uint32_t>(depth) << 24;
        // TODO: Add age
        payload |= static_cast<uint32_t>(bestMove.promote) << 17;
        payload |= static_cast<uint32_t>(bestMove.piece) << 14;
        payload |= static_cast<uint32_t>(bestMove.from) << 8;
        payload |= static_cast<uint32_t>(bestMove.to) << 2;
        payload |= static_cast<uint32_t>(nodeType);
    }
    bool IsValid() const { return hash > 0; }
    static PackedTableEntry Invalid() { return PackedTableEntry(); }
};

struct TableEntry {
    ZobristKey hash{0};
    int32_t score{0};
    uint8_t depth{0};
    Move bestMove{Move::Invalid()};
    NodeType::Value nodeType{NodeType::EXACT};

    TableEntry() = default;
    TableEntry(PackedTableEntry packed)
        : hash{packed.hash}, score{packed.score}
    {
        // 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 
        // ------- depth---------- ----age---- --promo- --piece- -----from-------  -------to-------  type

        depth = packed.payload >> 24;

        // TODO: Add age
        // uint8_t age = (packed.payload >> 20) & 0b1111;

        Piece::Value promote = static_cast<Piece::Value>((packed.payload >> 17) & 0b111);
        Piece::Value piece = static_cast<Piece::Value>((packed.payload >> 14) & 0b111);
        uint8_t from = (packed.payload >> 8) & 0b111111;
        uint8_t to = (packed.payload >> 2) & 0b111111;
        bestMove = Move(from, to, piece, promote);

        nodeType = static_cast<NodeType::Value>(packed.payload & 0b11);
    }
    bool IsValid() const { return hash > 0; }
    static TableEntry Invalid() { return TableEntry(); }
};

class TranspositionTable {
public:
    TranspositionTable();
    ~TranspositionTable();

    std::size_t OccupancyBytes() { return m_Occupancy * sizeof(PackedTableEntry); }
    TableEntry Get(ZobristKey key);
    void Save(const BoardState& state, int32_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType);

private:
    uint64_t Index(ZobristKey key) const;

private:
    std::size_t m_Occupancy{0};
    PackedTableEntry *m_Table{nullptr};
};
