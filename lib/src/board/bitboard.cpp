#include "bitboard.h"
#include <bit>
#include <sstream>
#include "core.h"
#include "util/exception.h"
#include "util/instrumenter.h"

void BitboardSet::StartPos()
{
    JUPITER_TRACE();

    m_Bits[Color::WHITE][Piece::PAWN]   = 0b00000000'11111111'00000000'00000000'00000000'00000000'00000000'00000000;
    m_Bits[Color::WHITE][Piece::KNIGHT] = 0b01000010'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
    m_Bits[Color::WHITE][Piece::BISHOP] = 0b00100100'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
    m_Bits[Color::WHITE][Piece::ROOK]   = 0b10000001'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
    m_Bits[Color::WHITE][Piece::QUEEN]  = 0b00001000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;
    m_Bits[Color::WHITE][Piece::KING]   = 0b00010000'00000000'00000000'00000000'00000000'00000000'00000000'00000000;

    m_Bits[Color::BLACK][Piece::PAWN]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'00000000;
    m_Bits[Color::BLACK][Piece::KNIGHT] = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'01000010;
    m_Bits[Color::BLACK][Piece::BISHOP] = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00100100;
    m_Bits[Color::BLACK][Piece::ROOK]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'10000001;
    m_Bits[Color::BLACK][Piece::QUEEN]  = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00001000;
    m_Bits[Color::BLACK][Piece::KING]   = 0b00000000'00000000'00000000'00000000'00000000'00000000'00000000'00010000;

    m_Combined[Color::WHITE] = 0b11111111'11111111'00000000'00000000'00000000'00000000'00000000'00000000;
    m_Combined[Color::BLACK] = 0b00000000'00000000'00000000'00000000'00000000'00000000'11111111'11111111;
}

void BitboardSet::Set(Color::Value color, Piece::Value piece, uint8_t index)
{
    JUPITER_TRACE();

    uint64_t bit = 1ul << index;
    m_Bits[color][piece] |= bit;
    m_Combined[color] |= bit;
}

void BitboardSet::Unset(Color::Value color, Piece::Value piece, uint8_t index)
{
    JUPITER_TRACE();

    uint64_t bit = 1ul << index;
    m_Bits[color][piece] &= ~bit;
    m_Combined[color] &= ~bit;
}

void BitboardSet::UnsetAll(Color::Value color, uint8_t index)
{
    JUPITER_TRACE();

    uint64_t bit = 1ul << index;
    if (m_Combined[color] & bit) {
        m_Bits[color][Piece::PAWN] &= ~bit;
        m_Bits[color][Piece::KNIGHT] &= ~bit;
        m_Bits[color][Piece::BISHOP] &= ~bit;
        m_Bits[color][Piece::ROOK] &= ~bit;
        m_Bits[color][Piece::QUEEN] &= ~bit;
        m_Bits[color][Piece::KING] &= ~bit;
        m_Combined[color] &= ~bit;
    }
}

void BitboardSet::Clear()
{
    JUPITER_TRACE();

    for (auto& board : m_Bits[Color::WHITE]) 
        board = 0ul;

    for (auto& board : m_Bits[Color::BLACK])
        board = 0ul;
}

bool BitboardSet::Has(Color::Value color, Piece::Value piece, uint8_t index) const
{
  StackInstrumenter _trace(
      __func__,
      "/home/angstrom/personal/dev/cpp/Jupiter-Chess-Interface/engines/Jupiter/"
      "lib/src/bitboard.cpp",
      74);

  uint64_t bit = 1ul << index;
  return m_Bits[color][piece] & bit;
}

bool BitboardSet::Has(Color::Value color, uint8_t index) const
{
    JUPITER_TRACE();

    uint64_t bit = 1ul << index;
    return m_Combined[color] & bit;
}

bool BitboardSet::Has(uint8_t index) const
{
    JUPITER_TRACE();

    uint64_t bit = 1ul << index;
    return (m_Combined[Color::WHITE] | m_Combined[Color::BLACK]) & bit;
}

bool BitboardSet::HasAny(const std::initializer_list<std::size_t>& indices) const 
{
    JUPITER_TRACE();

    for (const uint8_t index : indices) {
        uint64_t bit = 1ul << index;
        if ((m_Combined[Color::WHITE] | m_Combined[Color::BLACK]) & bit)
            return true;
    }
    return false;
}

uint8_t BitboardSet::Count(Color::Value color, Piece::Value piece) const 
{
    JUPITER_TRACE();

    return std::popcount(m_Bits[color][piece]);
}

Piece::Value BitboardSet::PieceInSquare(Color::Value color, uint8_t index) const 
{
    JUPITER_TRACE();

    if (Has(index)) {
        uint64_t bit = 1ul << index;
        for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
            if (m_Bits[color][piece] & bit)
                return piece;
        }
    }
    return Piece::Invalid();
}

std::pair<Color::Value, Piece::Value> BitboardSet::PieceInSquare(uint8_t index) const
{
    JUPITER_TRACE();

    if (Has(index)) {
        uint64_t bit = 1ul << index;
        for (const Color::Value color : { Color::WHITE, Color::BLACK }) {
            for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
                if (m_Bits[color][piece] & bit)
                    return std::make_pair(color, piece);
            }
        }
    }
    return std::make_pair(Color::Invalid(), Piece::Invalid());
}

Bitboard BitboardSet::OccupancyMask(Color::Value color, Piece::Value piece) const
{
    JUPITER_TRACE();

    return m_Bits[color][piece];
}

Bitboard BitboardSet::OccupancyMask(Color::Value color) const
{
    JUPITER_TRACE();

    return m_Combined[color];
}

Bitboard BitboardSet::OccupancyMask() const
{
    JUPITER_TRACE();

    return m_Combined[Color::WHITE] | m_Combined[Color::BLACK];
}

void BitboardSet::Show() const 
{
    JUPITER_TRACE();

    std::stringstream ss;

    const char symbols[Color::MAX_ENUM][Piece::MAX_ENUM] = {
        { 'P', 'N', 'B', 'R', 'Q', 'K' },
        { 'p', 'n', 'b', 'r', 'q', 'k' }
    };

    for (uint64_t i = 0; i < 64; i++) {
        if (i % 8 == 0)
            ss << std::endl << "    ";
        
        const auto [color, piece] = PieceInSquare(i);
        if (Color::IsValid(color) && Piece::IsValid(piece))
            ss << symbols[color][piece] << ' ';
        else
            ss << ". ";
    }
    INFO(ss.str());
}

void BitboardSet::Dump() const 
{
    JUPITER_TRACE();

    const auto& DisplayBitboard = [](Bitboard bitboard) {
        std::stringstream ss;
        for (std::size_t i = 0; i < 64; i++) {
            if (i % 8 == 0)
                ss << std::endl;

            ss << ((bitboard & (1ul << i)) ? "x " : ". ");
        }
        ss << std::endl;
        INFO(ss.str());
    };

    for (const Color::Value color : { Color::WHITE, Color::BLACK }) {
        for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
            INFO(Color::Show(color) << " " << Piece::Show(piece) << ":");
            DisplayBitboard(m_Bits[color][piece]);
        }
    }

    INFO("White combined");
    DisplayBitboard(m_Combined[Color::WHITE]);

    INFO("Black combined");
    DisplayBitboard(m_Combined[Color::BLACK]);
}

void BitboardSet::Validate() const 
{
    JUPITER_TRACE();

    if ((m_Combined[Color::WHITE] & m_Combined[Color::BLACK]) == 0)
        return;
        
    for (Piece::Value whitePiece = Piece::PAWN; whitePiece < Piece::MAX_ENUM; whitePiece++) {
        for (std::size_t i = 0; i < 64; i++) {
            for (Piece::Value blackPiece = Piece::PAWN; blackPiece < Piece::MAX_ENUM; blackPiece++) {
                uint64_t bit = 1ul << i;
                if ((m_Bits[Color::WHITE][whitePiece] & bit) && (m_Bits[Color::BLACK][blackPiece] & bit)) {
                    Dump();
                    Show();
                    std::stringstream ss;
                    ss << "Bitboards invalid: white " << Piece::Show(whitePiece) << " and black " << Piece::Show(blackPiece) << " both on square " << i;
                    throw JupiterException(ss.str());
                }
            }
        }
    }
}
