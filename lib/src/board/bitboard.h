#pragma once

#include "core.h"
#include <utility>

using Bitboard = uint64_t;

class BitboardSet {
public:
    void StartPos();
    void Set(Color::Value color, Piece::Value piece, uint8_t index);
    void Unset(Color::Value color, Piece::Value piece, uint8_t index);
    void UnsetAll(Color::Value color, uint8_t index);
    void Clear();
    bool Has(Color::Value color, Piece::Value piece, uint8_t index) const;
    bool Has(Color::Value color, uint8_t index) const;
    bool Has(uint8_t index) const;
    bool HasAny(const std::initializer_list<std::size_t>& indices) const;
    [[nodiscard]] uint8_t Count(Color::Value color, Piece::Value piece) const;
    [[nodiscard]] Piece::Value PieceInSquare(Color::Value color, uint8_t index) const;
    [[nodiscard]] std::pair<Color::Value, Piece::Value> PieceInSquare(uint8_t index) const;
    [[nodiscard]] Bitboard OccupancyMask(Color::Value color, Piece::Value piece) const;
    [[nodiscard]] Bitboard OccupancyMask(Color::Value color) const;
    [[nodiscard]] Bitboard OccupancyMask() const;

    void Show() const;
    void Dump() const;
    void Validate() const;

private:
    uint64_t m_Bits[Color::MAX_ENUM][Piece::MAX_ENUM]{{0}};
    uint64_t m_Combined[Color::MAX_ENUM]{0};
};
