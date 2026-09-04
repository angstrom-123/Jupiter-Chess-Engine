#include <charconv>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

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
        uint64_t length = std::strlen(fen);

        // Split at delimiters and initial validation
        FenView views[13];
        if (!SplitFEN(fen, length, views))
            throw JupiterException(std::string("Malformed FEN string: ") + fen);

        // Read piece positions
        {
            uint64_t boardIndex = 0;
            for (uint64_t i = 0; i < 8; i++) {
                const FenView& v = views[i];
                for (uint64_t j = v.start; j < v.end; j++) {
                    if (boardIndex >= 64)
                        throw JupiterException(std::string("Bad piece positions in FEN string: ") + fen);

                    char c = fen[j];
                    Color::Value color = (c > 'Z') ? Color::BLACK : Color::WHITE;
                    switch (c) {
                        case '1'...'8':
                            boardIndex += c - '0' - 1; // -1 because board index increments later too
                            break;
                        case 'p': case 'P':
                            m_State.pieces.Set(color, Piece::PAWN, boardIndex);
                            break;
                        case 'n': case 'N':
                            m_State.pieces.Set(color, Piece::KNIGHT, boardIndex);
                            break;
                        case 'b': case 'B':
                            m_State.pieces.Set(color, Piece::BISHOP, boardIndex);
                            break;
                        case 'r': case 'R':
                            m_State.pieces.Set(color, Piece::ROOK, boardIndex);
                            break;
                        case 'q': case 'Q':
                            m_State.pieces.Set(color, Piece::QUEEN, boardIndex);
                            break;
                        case 'k': case 'K':
                            m_State.pieces.Set(color, Piece::KING, boardIndex);
                            break;
                        default:
                            throw JupiterException(std::string("Bad piece positions in FEN string: ") + fen);
                    }
                    boardIndex++;
                }
            }
        }

        // Read color to move 
        {
            const FenView& v = views[8];
            if (v.end - v.start > 1)
                throw JupiterException("Bad turn to move in FEN string");
            if (fen[v.start] == 'w')
                m_State.turn = Color::WHITE;
            else if (fen[v.start] == 'b')
              m_State.turn = Color::BLACK;
            else
              throw JupiterException("Bad turn to move in FEN string");
        }

        // Read castling rights
        {
            const FenView& v = views[9];
            m_State.rights = 0;

            if (fen[v.start] != '-') {
                for (uint64_t i = v.start; i < v.end; i++) {
                    char c = fen[i];
                    switch (c) {
                        case 'K':
                            m_State.rights |= CastlingRight::KINGSIDE_WHITE;
                            break;
                        case 'k':
                            m_State.rights |= CastlingRight::KINGSIDE_BLACK;
                            break;
                        case 'Q':
                            m_State.rights |= CastlingRight::QUEENSIDE_WHITE;
                            break;
                        case 'q':
                            m_State.rights |= CastlingRight::QUEENSIDE_BLACK;
                            break;
                        default:
                            throw JupiterException(std::string("Bad castling rights in FEN string: ") + fen);
                    }
                }
            }
        }

        // Read en-passant square
        {
            const FenView& v = views[10];
            if (v.end - v.start == 1 && fen[v.start] == '-') {
                m_State.enPassantIndex = UINT8_MAX;
            } else if (v.end - v.start == 2) {
                char first = fen[v.start];
                char second = fen[v.start + 1];
                if (first < 'a' || first > 'h' || second < '1' || second > '8')
                    throw JupiterException(std::string("Bad en passant square in FEN string: ") + fen);
                m_State.enPassantIndex = first - 'a' + 8 * (7 - (second - '1'));
            } else {
                throw JupiterException(std::string("Bad en passant square in FEN string: ") + fen);
            }
        }

        // Read half-move counter 
        {
            const FenView& v = views[11];
            auto res = std::from_chars(&fen[v.start], &fen[v.end], m_HalfMoves);
            if (res.ec != std::errc())
                throw JupiterException(std::string("Bad half move counter in FEN string: ") + fen);
        }

        // Read full-move counter 
        {
            const FenView& v = views[12];
            auto res = std::from_chars(&fen[v.start], &fen[v.end], m_FullMoves);
            if (res.ec != std::errc())
                throw JupiterException(std::string("Bad half move counter in FEN string: ") + fen);
        }

        // Save initial board state to history
        m_State.zobristKey = m_Zobrist.ComputeKey(m_State);
        m_History.Push(m_State);
        ComputePSTScore();
    }

    Board::~Board()
    {
        JUPITER_PROFILING_END();
    }

    // Now doing incremental updates so need to initialise it
    void Board::ComputePSTScore()
    {
        JUPITER_TRACE();
        JUPITER_PROFILE();

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
        JUPITER_PROFILE();

        m_Searcher.SetTimeControl(seconds, increment);
    }

    Move Board::Go(uint64_t moveMs)
    {
        JUPITER_TRACE();
        JUPITER_PROFILE();

        return m_Searcher.FindBest(m_State, m_History, moveMs);
    }

    void Board::MakeMove(LongAlgebraicMove lan)
    {
        JUPITER_TRACE();
        JUPITER_PROFILE();

        Move move = Move::FromLAN(lan, m_State.pieces);
        if (!move.IsValid())
            throw JupiterException(std::string("Malformed LAN string: ") + lan.chars);

        m_Searcher.MakeMove(m_State, move);
        m_History.Push(m_State);

        m_HalfMoves++;
        if (m_State.turn == Color::WHITE)
            m_FullMoves++;
    }

    void Board::GetTelemetry(std::string& result)
    {
        JUPITER_TRACE();
        JUPITER_PROFILE();

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
        JUPITER_PROFILE();

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
        JUPITER_PROFILE();

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
        JUPITER_PROFILE();

        m_State.pieces = BitboardSet{};
        m_State.rights = CastlingRights{};
        m_State.turn = Color::WHITE;
        m_State.enPassantIndex = UINT8_MAX;
        m_HalfMoves = 0;
        m_FullMoves = 1;
    }

    bool Board::SplitFEN(const char *fen, uint64_t length, FenView (&views)[13]) 
    {
        JUPITER_TRACE();
        JUPITER_PROFILE();

        // Verify that the string is well formed
        uint64_t spaceCounter = 0;
        uint64_t slashCounter = 0;
        for (uint64_t i = 0; i < length; i++) {
            char c = fen[i];
            switch (c) {
                case ' ':
                    spaceCounter++;
                    break;
                case '/':
                    slashCounter++;
                    break;
                case 'p': case 'P':
                case 'n': case 'N':
                case 'B': // case 'b' handled in the 'a'-'h' check
                case 'r': case 'R':
                case 'q': case 'Q':
                case 'k': case 'K':
                case 'w': case '-':
                case 'a'...'h':
                case '0'...'9':
                    break;
                default:
                    ERROR("Invalid character: " << c);
                    return false;
            }
        }
        if (spaceCounter != 5 || slashCounter != 7) {
            ERROR("Invalid space or slash count: " << spaceCounter << " spaces (expected 5), " << slashCounter << " slashes (expected 7)");
            return false;
        }

        // Split at delimiting characters (spaces and forward slashes)
        uint64_t viewCounter = 0;
        uint64_t start = 0;
        for (uint64_t end = 1; end < length; end++) {
            if (fen[end] == ' ' || fen[end] == '/') {
                views[viewCounter++] = { start, end };
                start = end + 1;
            }
        }
        views[viewCounter++] = { start, length };
        if (viewCounter != 13) {
            ERROR("Invalid block count: " << viewCounter << " (expected 13)");
            return false;
        }

        return true;
    }
}
