#pragma once

#include <atomic>
#include <cstdint>
#include "evaluation/evaluator.h"
#include "movegen/move.h"
#include "board/zobrist.h"

// constexpr uint64_t TRANSPOSITION_TABLE_KEY_BITS = 25; // 512 MiB
constexpr uint64_t TRANSPOSITION_TABLE_KEY_BITS = 23; // 128 MiB
constexpr std::size_t TRANSPOSITION_TABLE_SIZE = 1ull << TRANSPOSITION_TABLE_KEY_BITS;

// Memory prefetch with funky fallback. I don't expect it to run, but it's there 
// anyway.
INLINE void Prefetch(const void * p)
{
    #if defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(p, 0, 3);
    #elif defined(_MSC_VER)
        _mm__prefetch(reinterpret_cast<const char *>(p), _MM_HINT_T0);
    #else
        #warning Fallback prefetch used on this platform
        volatile const uint8_t *v = reinterpret_cast<volatile const uint8_t *>(p);
        (void) *v;
    #endif
}

struct Cutoff {
    typedef enum : uint8_t {
        NONE = 0,
        ALPHA = 1,
        BETA = 2
    } Value;
};

struct PackedAtomics {
    // Zobrist key XOR data, used for XOR trick
    std::atomic<uint64_t> key{0};

    // Top 32 bits are the score, lower 32 bits are packed as follows:
    //
    // 32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 
    // ------- depth---------- ----age---- --promo- --piece- -----from-------  -------to------- cutoff
    std::atomic<uint64_t> data{0};
};

struct TranspositionTableEntry {
    ZobristKey hash{0};
    int32_t score{0};
    Move bestMove{Move::Invalid()};
    uint16_t halfMove{0};
    uint8_t depth{0};
    Cutoff::Value cutoff{Cutoff::NONE};

    INLINE static constexpr TranspositionTableEntry Invalid() { return TranspositionTableEntry {}; }
    INLINE bool IsValid() { return bestMove.IsValid(); } // TT probing in search relies on this validating the bestmove validity
};

class TranspositionTable {
public:
    TranspositionTable();
    ~TranspositionTable();
    void Store(TranspositionTableEntry entry);
    TranspositionTableEntry Probe(ZobristKey hash) const;
    INLINE const void *Line(ZobristKey hash) const { return &m_Table[Index(hash)]; }
    std::size_t OccupancyBytes() const;
    INLINE static Cutoff::Value ClassifyCutoff(int32_t bestScore, int32_t startAlpha, int32_t windowBeta)
    {
        if (bestScore >= windowBeta) return Cutoff::BETA;
        if (bestScore <= startAlpha) return Cutoff::ALPHA;
        return Cutoff::NONE;
    }
    INLINE static int32_t NormalizeScore(int32_t bestScore, uint8_t ply)
    {
        if (bestScore > MATE_THRESHOLD) return bestScore + ply;
        if (bestScore < -MATE_THRESHOLD) return bestScore - ply;
        return bestScore;
    }
    INLINE static int32_t DenormalizeScore(int32_t score, uint8_t ply)
    {
        if (score > MATE_THRESHOLD) return score - ply;
        if (score < -MATE_THRESHOLD) return score + ply;
        return score;
    }

private:
    TranspositionTableEntry Decode(uint64_t key, uint64_t data) const;
    INLINE std::size_t Index(ZobristKey hash) const { return hash & (TRANSPOSITION_TABLE_SIZE - 1); }

private:
    PackedAtomics *m_Table{nullptr};
};
