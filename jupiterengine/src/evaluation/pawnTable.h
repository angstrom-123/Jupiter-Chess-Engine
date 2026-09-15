#pragma once 

#include "board/bitboard.h"
#include "board/zobrist.h"

constexpr uint64_t PAWN_TABLE_KEY_BITS = 16;
constexpr std::size_t PAWN_TABLE_SIZE = 1ull << PAWN_TABLE_KEY_BITS;

// Packs the indices of the furthest pushed pawn in each rank relative to color
struct PawnRanks {
    uint8_t pairs[4]{0b10001000, 0b10001000, 0b10001000, 0b10001000 };

    void Pack(uint8_t file, uint8_t rank)
    {
        uint8_t& pair = pairs[file / 2];
        uint8_t shift = 4 * (file & 1);
        pair &= 0b00001111 << (4 - shift); // Zero out the bits that we are packing to
        pair |= (rank << shift);           // Set the rank
    }

    uint8_t Rank(uint8_t file) const 
    {
        // 8  7  6  5  4  3  2  1
        // s  --n+1--  s  ---n---

        uint8_t pair = pairs[file / 2];
        uint8_t shift = 4 * (file & 1);
        uint8_t bits = (pair >> shift) & 0b1111;
        if (bits & 0b1000)
            return UINT8_MAX;
        return bits & 0b0111;
    }
};

struct PawnStructure {
    struct Relative {
        Bitboard weak{0};         // King pawn tropism
        Bitboard passed{0};       // King pawn tropism
        PawnRanks furthest{};     // Pawn shield / storm
    } relative[Color::MAX_ENUM];
    Bitboard openFiles{0};        // Rook / Queen bonus
};

struct PTEntry {
    ZobristKey hash{0};
    int32_t score{0};
    PawnStructure structure{PawnStructure{}};

    PTEntry() = default;
    PTEntry(ZobristKey hash, int32_t score, PawnStructure& structure)
        : hash{hash}, score{score}, structure{structure} {}
    static PTEntry Invalid() { return PTEntry{}; }
    bool IsValid() const { return hash > 0; }
};

class PawnTable {
public:
    PawnTable();
    ~PawnTable();
    std::size_t OccupancyBytes() const { return m_Occupancy * sizeof(PTEntry); }
    PTEntry Get(ZobristKey pawnKey) const;
    void Save(ZobristKey pawnKey, int32_t score, PawnStructure& structure);

private:
    INLINE std::size_t Index(ZobristKey key) const { return key & (PAWN_TABLE_SIZE - 1); }

private:
    std::size_t m_Occupancy{0};
    PTEntry *m_Table{nullptr};
};
