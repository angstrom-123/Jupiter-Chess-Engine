#include "bitboard.h"
#include <sstream>
#include "core.h"
#include "util/exception.h"
#include "util/instrumenter.h"

BitboardSet::BitboardSet()
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

void BitboardSet::Clear()
{
    JUPITER_TRACE();

    for (Bitboard& board : m_Bits[Color::WHITE]) 
        board = 0ull;

    for (Bitboard& board : m_Bits[Color::BLACK])
        board = 0ull;

    for (Bitboard& board : m_Combined)
        board = 0ull;
}

Piece::Value BitboardSet::PieceInSquare(Color::Value color, uint8_t index) const 
{
    JUPITER_TRACE();

    if (Has(color, index)) {
        uint64_t bit = 1ull << index;
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
        uint64_t bit = 1ull << index;
        for (const Color::Value color : { Color::WHITE, Color::BLACK }) {
            for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
                if (m_Bits[color][piece] & bit)
                    return std::make_pair(color, piece);
            }
        }
    }
    return std::make_pair(Color::Invalid(), Piece::Invalid());
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

    for (const Color::Value color : { Color::WHITE, Color::BLACK }) {
        for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
            INFO(Color::Show(color) << " " << Piece::Show(piece) << ":");
            ShowBitboard(m_Bits[color][piece]);
        }
    }

    INFO("White combined");
    ShowBitboard(m_Combined[Color::WHITE]);

    INFO("Black combined");
    ShowBitboard(m_Combined[Color::BLACK]);
}

void BitboardSet::Validate() const 
{
    JUPITER_TRACE();

    if ((m_Combined[Color::WHITE] & m_Combined[Color::BLACK]) == 0)
        return;
        
    for (Piece::Value whitePiece = Piece::PAWN; whitePiece < Piece::MAX_ENUM; whitePiece++) {
        for (std::size_t i = 0; i < 64; i++) {
            for (Piece::Value blackPiece = Piece::PAWN; blackPiece < Piece::MAX_ENUM; blackPiece++) {
                uint64_t bit = 1ull << i;
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
