#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "util/fenParser.h"
#include "libjupiter/board.h"
#include "core.h"
#include "util/exception.h"
#include "util/instrumenter.h"
#include "movegen/move.h"
#include "zobrist.h"

namespace libjupiter {
    Board::Board(const char *fen)
    {
        JUPITER_TRACE();

        if (fen == nullptr)
        {
            m_State.pieces.StartPos();
            m_State.rights = CastlingRight::KINGSIDE_BLACK | 
                CastlingRight::KINGSIDE_WHITE |
                CastlingRight::QUEENSIDE_BLACK |
                CastlingRight::QUEENSIDE_WHITE,
            m_State.turn = Color::WHITE;
            m_State.enPassantIndex = UINT8_MAX;
            m_State.zobristKey = m_Zobrist.ComputeKey(m_State);
            ComputePSTScore();
            return;
        }

        Clear();
        fen::Parse(fen, &m_HalfMoves, &m_FullMoves, m_State);

        // Save initial board state to history
        m_State.zobristKey = m_Zobrist.ComputeKey(m_State);
        m_History.Push(m_State);
        ComputePSTScore();
    }

    // Now doing incremental updates so need to initialise it
    void Board::ComputePSTScore()
    {
        JUPITER_TRACE();

        m_State.pstScore = PSTScore(0, 0);
        for (Color::Value color : { Color::WHITE, Color::BLACK }) {
            for (Piece::Value piece = Piece::PAWN; piece < Piece::MAX_ENUM; piece++) {
                Bitboard occupancy = m_State.pieces.OccupancyMask(color, piece);
                while (occupancy) {
                    uint8_t index = std::countr_zero(occupancy);
                    m_State.pstScore += m_PieceSquareTables.Get(color, piece, index);
                    occupancy &= (occupancy - 1);
                }
            }
        }
    }

    void Board::SetTimeControl(uint64_t seconds, uint64_t increment)
    {
        JUPITER_TRACE();

        m_Searcher.SetTimeControl(seconds, increment);
    }

    Move Board::Go(uint64_t moveMs)
    {
        JUPITER_TRACE();

        return m_Searcher.FindBest(m_State, m_History, moveMs);
    }

    void Board::MakeMove(LongAlgebraicMove lan)
    {
        JUPITER_TRACE();

        Move move = Move::FromLAN(lan, m_State.pieces);
        if (!move.IsValid())
            throw JupiterException(std::string("Malformed LAN string: ") + lan.chars);

        m_State.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        m_History.Push(m_State);

        m_HalfMoves++;
        if (m_State.turn == Color::WHITE)
            m_FullMoves++;
    }

    void Board::GetTelemetry(std::string& result)
    {
        JUPITER_TRACE();

        std::ostringstream ss;

        ss << "{"
            << "\"depth\":" << (int) m_Searcher.searchDepth << ","
            << "\"nodesSearched\":" << m_Searcher.nodesSearched << ","
            << "\"nodesLookedUp\":" << m_Searcher.nodesLookedUp << ","
            << "\"nodesQuiesced\":" << m_Searcher.nodesQuiesced << ","
            << "\"searchTime\":" << m_Searcher.searchTime 
            << "}";

        result = ss.str();
    }

    void Board::GetMetrics(std::string& result)
    {
        JUPITER_TRACE();

        std::ostringstream ss;

        ss << "{"
            << "\"ttSize\":" << m_Searcher.ttSize << ","
            << "\"bookMoves\":" << (int) m_Searcher.bookMoves
            << "}";

        result = ss.str();
    }

    void Board::Show(std::string& result)
    {
        JUPITER_TRACE();

        std::ostringstream ss;
        ss << "Move " << m_FullMoves << std::endl
            << (m_State.turn == Color::BLACK ? "Black" : "White") << " to move" << std::endl
            << "Castling rights: " << std::endl
            << "    White long: " << ((m_State.rights & CastlingRight::QUEENSIDE_WHITE) ? "true" : "false")
            << ", short: " << ((m_State.rights & CastlingRight::KINGSIDE_WHITE) ? "true" : "false") << std::endl
            << "    Black long: " << ((m_State.rights & CastlingRight::QUEENSIDE_BLACK) ? "true" : "false")
            << ", short: " << ((m_State.rights & CastlingRight::KINGSIDE_BLACK) ? "true" : "false") << std::endl
            << "Board: ";

        const char symbols[Color::MAX_ENUM][Piece::MAX_ENUM] = {
            { 'P', 'N', 'B', 'R', 'Q', 'K' },
            { 'p', 'n', 'b', 'r', 'q', 'k' }
        };

        for (uint64_t i = 0; i < 64; i++) {
            if (i % 8 == 0)
                ss << std::endl << "    ";
            
            const auto [color, piece] = m_State.pieces.PieceInSquare(i);
            if (Color::IsValid(color) && Piece::IsValid(piece))
                ss << symbols[color][piece] << ' ';
            else
                ss << ". ";
        }
        result = ss.str();
    }

    void Board::Clear()
    {
        JUPITER_TRACE();

        m_State.pieces = BitboardSet{};
        m_State.rights = CastlingRights{};
        m_State.turn = Color::WHITE;
        m_State.enPassantIndex = UINT8_MAX;
        m_HalfMoves = 0;
        m_FullMoves = 1;
    }
}
