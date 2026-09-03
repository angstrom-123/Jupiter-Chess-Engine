#pragma once

#include <cstdint>
#include "movegen/move.h"
#include "board/boardState.h"
#include "board/zobrist.h"

constexpr uint64_t TRANSPOSITION_TABLE_KEY_BITS = 25;
constexpr std::size_t TRANSPOSITION_TABLE_SIZE = 1ul << TRANSPOSITION_TABLE_KEY_BITS;

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

struct TableEntry {
    ZobristKey hash{0};
    int64_t score{0};
    Move bestMove{Move::Invalid()};
    uint8_t depth{0};
    NodeType::Value nodeType{NodeType::EXACT};

    bool IsValid() const { return hash > 0; }
    static TableEntry Invalid() { return TableEntry{ .hash = 0 }; }
};

class TranspositionTable {
public:
    TranspositionTable();
    ~TranspositionTable();

    std::size_t OccupancyBytes() { return m_Occupancy * sizeof(TableEntry); }
    TableEntry Get(ZobristKey key);
    void Save(const BoardState& state, int64_t score, uint8_t depth, Move bestMove, NodeType::Value nodeType);

private:
    uint64_t Index(ZobristKey key) const;

private:
    std::size_t m_Occupancy{0};
    TableEntry *m_Table{nullptr};
};
