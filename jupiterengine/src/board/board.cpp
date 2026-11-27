#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "evaluation/evaluator.h"
#include "libjupiter/board.h"
#include "core.h"
#include "util/exception.h"
#include "util/fenParser.h"
#include "util/instrumenter.h"
#include "movegen/move.h"
#include "zobrist.h"

namespace libjupiter {
    Board::Board(const char *_Nullable fen)
    {
        JUPITER_TRACE();

        m_DTSManager = new DTSManager();
        fen::Parse(fen ? fen : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", nullptr, nullptr, m_DTSManager->state);
    }

    Board::~Board()
    {
        INFO("Cleaning up board");
        delete m_DTSManager;
        INFO("Clean up done");
    }

    void Board::SetTimeControl(float seconds, float increment)
    {
        JUPITER_TRACE();

        m_DTSManager->tcSeconds = seconds;
        m_DTSManager-> tcIncrement = increment;
    }

    Move Board::Go(uint64_t moveMs)
    {
        JUPITER_TRACE();

        return m_DTSManager->FindBest(moveMs);
    }

    void Board::MakeMove(LongAlgebraicMove lan)
    {
        JUPITER_TRACE();

        Move move = Move::FromLAN(lan, m_DTSManager->state.pieces);
        if (!move.IsValid())
            throw JupiterException(std::string("Malformed LAN string: '") + lan.chars + "'");

        m_DTSManager->state.MakeMoveNoIncremental(move);
    }

    void Board::SetWeights(const EvaluatorConstants& weights)
    {
        JUPITER_TRACE();

        m_DTSManager->weights = weights;
    }

    EvaluatorConstants Board::GetWeights() const
    {
        JUPITER_TRACE();

        return m_DTSManager->weights;
    }

    void Board::GetTelemetry(std::string& result) const
    {
        JUPITER_TRACE();

        m_DTSManager->TelemetryJSON(result);
    }

    void Board::GetMetrics(std::string& result) const
    {
        JUPITER_TRACE();

        m_DTSManager->MetricsJSON(result);
    }

    void Board::Show(std::string& result) const
    {
        JUPITER_TRACE();

        std::ostringstream ss;
        ss << (m_DTSManager->state.turn == Color::BLACK ? "Black" : "White") << " to move" << std::endl
            << "Castling rights: " << std::endl
            << "    White long: " << ((m_DTSManager->state.rights & CastlingRight::QUEENSIDE_WHITE) ? "true" : "false")
            << ", short: " << ((m_DTSManager->state.rights & CastlingRight::KINGSIDE_WHITE) ? "true" : "false") << std::endl
            << "    Black long: " << ((m_DTSManager->state.rights & CastlingRight::QUEENSIDE_BLACK) ? "true" : "false")
            << ", short: " << ((m_DTSManager->state.rights & CastlingRight::KINGSIDE_BLACK) ? "true" : "false") << std::endl
            << "Board: ";

        const char symbols[Color::MAX_ENUM][Piece::MAX_ENUM] = {
            { 'P', 'N', 'B', 'R', 'Q', 'K' },
            { 'p', 'n', 'b', 'r', 'q', 'k' }
        };

        for (uint64_t i = 0; i < 64; i++) {
            if (i % 8 == 0)
                ss << std::endl << "    ";
            
            const auto [color, piece] = m_DTSManager->state.pieces.PieceInSquare(i);
            if (Color::IsValid(color) && Piece::IsValid(piece))
                ss << symbols[color][piece] << ' ';
            else
                ss << ". ";
        }
        result = ss.str();
    }
}
