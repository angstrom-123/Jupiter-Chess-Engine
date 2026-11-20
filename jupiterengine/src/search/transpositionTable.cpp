#include "transpositionTable.h"
#include <atomic>
#include <cassert>
#include <cstring>
#include "util/instrumenter.h"

#ifdef DEBUG
    #include "util/exception.h"
#endif

TranspositionTable::TranspositionTable()
{
    JUPITER_TRACE();

    m_Table = new PackedAtomics[TRANSPOSITION_TABLE_SIZE]{};
}

TranspositionTable::~TranspositionTable()
{
    delete[] m_Table;
}

void TranspositionTable::Store(TranspositionTableEntry entry)
{
    JUPITER_TRACE();

    std::size_t i = Index(entry.hash);
    uint64_t key = m_Table[i].key.load(std::memory_order_relaxed);
    uint64_t data = m_Table[i].data.load(std::memory_order_relaxed);

    bool store = true;
    if (data > 0) { // Set to 0 on table init, any write will change it (never back to 0)
        TranspositionTableEntry old = Decode(key ^ data, data);

        // Only overwrite old entry if the depth is greater or is much fresher.
        // The score coefficients are arbitrary and should be tuned
        uint8_t ageDifference = (entry.halfMove - old.halfMove) & 0b1111;
        int32_t oldScore = old.depth * 3 - ageDifference;
        int32_t newScore = entry.depth * 3;
        store = newScore >= oldScore;
    }

    if (store) {
        uint64_t data = 0;
        data |= static_cast<uint64_t>(std::bit_cast<uint32_t>(entry.score)) << 32; // Don't lose the sign bit off the top
        data |= static_cast<uint64_t>(entry.depth & 0b11111111) << 24;
        data |= static_cast<uint64_t>(entry.halfMove & 0b1111) << 20;
        data |= static_cast<uint64_t>(entry.bestMove.Pack()) << 2;
        data |= static_cast<uint64_t>(entry.cutoff & 0b11);

        // XOR trick, lets us check that both "halves" of the data are read together
        uint64_t key = entry.hash ^ data;

        m_Table[i].data.store(data, std::memory_order_relaxed);
        m_Table[i].key.store(key, std::memory_order_relaxed);
    }
}

TranspositionTableEntry TranspositionTable::Probe(ZobristKey hash) const
{
    JUPITER_TRACE();

    std::size_t i = Index(hash);
    uint64_t key = m_Table[i].key.load(std::memory_order_relaxed);
    uint64_t data = m_Table[i].data.load(std::memory_order_relaxed);

    // XOR trick, if this doesn't match then another thread wrote to this while we were reading.
    if (data == 0 || (key ^ data) != hash)
        return TranspositionTableEntry::Invalid();

    return Decode(hash, data);
}

std::size_t TranspositionTable::OccupancyBytes() const 
{
    const std::size_t SAMPLE_SIZE = 4096;

    std::size_t occupiedSlots = 0;
    for (std::size_t i = 0; i < SAMPLE_SIZE; i++) {
        if (m_Table[i].data > 0)
            occupiedSlots++;
    }
    return sizeof(PackedAtomics) * occupiedSlots * TRANSPOSITION_TABLE_SIZE / SAMPLE_SIZE;
}

TranspositionTableEntry TranspositionTable::Decode(uint64_t key, uint64_t data) const
{
    JUPITER_TRACE();

    TranspositionTableEntry entry {};
    entry.hash = key;
    entry.score = std::bit_cast<int32_t>(static_cast<uint32_t>(data >> 32)); // 2 stage cast to preserve sign bit
    entry.depth = static_cast<uint8_t>((data >> 24) & 0b11111111);
    entry.halfMove = static_cast<uint8_t>((data >> 20) & 0b1111);
    entry.bestMove = Move::Unpack((data >> 2)); // Don't need to mask it off, handled in Unpack()
    entry.cutoff = static_cast<Cutoff::Value>(data & 0b11);

#ifdef DEBUG 
    Move m = entry.bestMove;
    if (m.from > 63 || m.to > 63 || !Piece::IsValid(m.piece)) {
        ERROR("Invalid TT move: ");
        ERROR("  from: " << (int) m.from);
        ERROR("  to: " << (int) m.to);
        ERROR("  piece: " << Piece::Show(m.piece));
        ERROR("  promote: " << Piece::Show(m.promote));
        ERROR("Data: " << data);

        throw JupiterException("Invalid TT move");
    }
#endif

    return entry;
}
