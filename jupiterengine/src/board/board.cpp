#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "evaluation/evaluator.h"
#include "libjupiter/board.h"
#include "core.h"
#include "util/exception.h"
#include "util/instrumenter.h"
#include "movegen/move.h"

namespace libjupiter {
    Board::Board(const char *fen)
    {
        JUPITER_TRACE();

        m_DTSManager = new DTSManager(fen ? fen : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    void Board::SetTimeControl(float seconds, float increment)
    {
        JUPITER_TRACE();

        m_Searcher->SetTimeControl(seconds, increment);
    }

    Move Board::Go(uint64_t moveMs)
    {
        JUPITER_TRACE();

	return m_Searcher->FindBest(m_State, m_History, moveMs);
    }

    void Board::MakeMove(LongAlgebraicMove lan)
    {
        JUPITER_TRACE();

        Move move = Move::FromLAN(lan, m_State.pieces);
        if (!move.IsValid())
            throw JupiterException(std::string("Malformed LAN string: '") + lan.chars + "'");

        m_State.MakeMove(m_Zobrist, m_PieceSquareTables, move);
        m_History.Push(m_State);

        m_HalfMoves++;
        if (m_State.turn == Color::WHITE)
            m_FullMoves++;
    }

    void Board::SetWeights(const EvaluatorConstants& weights)
    {
        JUPITER_TRACE();

        m_Searcher->TuneEval(weights);
    }

    EvaluatorConstants Board::GetWeights() const
    {
        JUPITER_TRACE();

        return m_Searcher->EvalWeights();
    }

    void Board::GetTelemetry(std::string& result) const
    {
        JUPITER_TRACE();

        m_Searcher->TelemetryJSON(result);
    }

    void Board::GetMetrics(std::string& result) const
    {
        JUPITER_TRACE();

        m_Searcher->MetricsJSON(result);
    }

	uint64_t Board::Perft(uint8_t depth)
	{
		JUPITER_TRACE();

		return m_Searcher->Perft(m_State, depth);
	}

	uint64_t Board::PerftDivide(uint8_t depth, PerftBuffer& buffer)
	{
		JUPITER_TRACE();

		return m_Searcher->PerftDivide(m_State, depth, buffer);
	}

    void Board::Show(std::string& result) const
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
}
