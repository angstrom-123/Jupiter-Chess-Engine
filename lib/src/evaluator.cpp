#include "evaluator.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include "buffer.h"
#include "instrumenter.h"
#include "movegen.h"

int64_t Evaluator::Evaluate(const BoardState& state) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    int64_t eval = 0;

    eval += MaterialBalance(std::forward<const BoardState>(state));
    eval += PiecePositions(std::forward<const BoardState>(state));

    return eval;
}

int64_t Evaluator::MaterialBalance(const BoardState& state) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    // Draw by fifty move rule
    if (state.fiftyMoveCounter >= 75)
        return 0;

    int64_t materialEval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    int64_t nPawns = state.pieces.Count(friendly, Piece::PAWN) - state.pieces.Count(enemy, Piece::PAWN);
    int64_t nKnights = state.pieces.Count(friendly, Piece::KNIGHT) - state.pieces.Count(enemy, Piece::KNIGHT);
    int64_t nBishops = state.pieces.Count(friendly, Piece::BISHOP) - state.pieces.Count(enemy, Piece::BISHOP);
    int64_t nRooks = state.pieces.Count(friendly, Piece::ROOK) - state.pieces.Count(enemy, Piece::ROOK);
    int64_t nQueens = state.pieces.Count(friendly, Piece::QUEEN) - state.pieces.Count(enemy, Piece::QUEEN);

    materialEval = (nPawns * Piece::Evaluate(Piece::PAWN)) 
        + (nKnights * Piece::Evaluate(Piece::KNIGHT)) 
        + (nBishops * Piece::Evaluate(Piece::BISHOP)) 
        + (nRooks * Piece::Evaluate(Piece::ROOK)) 
        + (nQueens * Piece::Evaluate(Piece::QUEEN));

    return materialEval;
}

int64_t Evaluator::PiecePositions(const BoardState& state) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    int64_t piecePositionEval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    uint8_t phase = GamePhase(std::forward<const BoardState>(state));

    for (uint8_t i = Piece::PAWN; i < Piece::MAX_ENUM; i++) {
        Piece::Value piece = static_cast<Piece::Value>(i);

        Bitboard friendlyoccupancy = state.pieces.OccupancyMask(friendly, piece);
        while (friendlyoccupancy) {
            uint8_t index = std::countr_zero(friendlyoccupancy);
            piecePositionEval += m_PieceSquareTables.Get(friendly, piece, index, phase);
            friendlyoccupancy &= (friendlyoccupancy - 1);
        }

        Bitboard enemyoccupancy = state.pieces.OccupancyMask(enemy, piece);
        while (enemyoccupancy) {
            uint8_t index = std::countr_zero(enemyoccupancy);
            piecePositionEval -= m_PieceSquareTables.Get(enemy, piece, index, phase);
            enemyoccupancy &= (enemyoccupancy - 1);
        }
    }

    return piecePositionEval;
}

int64_t Evaluator::Mobility(const BoardState& state, const AttackTable& table) const 
{
    // TODO: Uncomment this, need a way first to test the engine though
    return 0;
    // Movegen movegen(state, table);
    // std::size_t attackCount = movegen.GetAttacks().Size();
    // std::size_t quietCount = movegen.GetQuiets().Size();
    // const int64_t MOVE_SCORE = 10;
    // return (attackCount + quietCount) * MOVE_SCORE;
}

// 0-100, higher = more likely to be endgame
uint8_t Evaluator::GamePhase(const BoardState& state) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    uint8_t score = 0;

    constexpr float MINOR_PIECE_WEIGHT = 30.0;
    constexpr float MAJOR_PIECE_WEIGHT = 40.0;
    constexpr float PAWN_WEIGHT = 10.0;
    constexpr float MOVE_WEIGHT = 20.0;

    static_assert(MINOR_PIECE_WEIGHT + MAJOR_PIECE_WEIGHT + PAWN_WEIGHT + MOVE_WEIGHT == 100.0, "Weights must sum to 100.");

    // Minor piece counts
    uint8_t nKnights = state.pieces.Count(Color::WHITE, Piece::KNIGHT) + state.pieces.Count(Color::BLACK, Piece::KNIGHT);
    uint8_t nBishops = state.pieces.Count(Color::WHITE, Piece::BISHOP) + state.pieces.Count(Color::BLACK, Piece::BISHOP);
    score += std::floor(MINOR_PIECE_WEIGHT / static_cast<float>(std::max(nKnights + nBishops, 1)));

    // Major piece counts
    uint8_t nRooks = state.pieces.Count(Color::WHITE, Piece::ROOK) + state.pieces.Count(Color::BLACK, Piece::ROOK);
    uint8_t nQueens = state.pieces.Count(Color::WHITE, Piece::QUEEN) + state.pieces.Count(Color::BLACK, Piece::QUEEN);
    score += std::floor(MAJOR_PIECE_WEIGHT / static_cast<float>(std::max(nRooks + nQueens, 1)));

    // Pawn counts
    uint8_t nPawns = state.pieces.Count(Color::WHITE, Piece::PAWN) + state.pieces.Count(Color::BLACK, Piece::PAWN);
    score += std::floor(PAWN_WEIGHT / static_cast<float>(std::max(nPawns, static_cast<uint8_t>(1))));

    // Move number
    score += std::floor(MOVE_WEIGHT / static_cast<float>(std::max(state.halfMoves / 2, 1)));

    return score;
}

int64_t Evaluator::SEE(const BoardState& state, Move move) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    BitboardSet pieces(state.pieces);
    Color::Value enemy = Color::Opposite(state.turn);

    // Simulate first capture
    Piece::Value firstCapture = pieces.PieceInSquare(enemy, move.to);
    if (!Piece::IsValid(firstCapture)) // en passant
        firstCapture = pieces.PieceInSquare(enemy, (enemy == Color::WHITE) ? move.to - 8 : move.to + 8);

    pieces.Unset(enemy, firstCapture, move.to);
    pieces.Unset(state.turn, move.piece, move.from);
    pieces.Set(state.turn, move.piece, move.to);

    Buffer<int64_t, 16> gain;
    gain.PushBack(Piece::Evaluate(firstCapture));

    // TODO: Don't recalculate attackers at each iteration, just update the bitboards 
    //       iteratively at each step. Then only sliders need recalculation (in case of discovery).

    Color::Value turn = enemy;
    Piece::Value target = move.piece;
    while (true) {
        Color::Value opponentTurn = Color::Opposite(turn);

        // Find attackers
        Bitboard attackers[Color::MAX_ENUM][Piece::MAX_ENUM];
        for (uint8_t i = Color::WHITE; i < Color::MAX_ENUM; i++) {
            Color::Value friendly = static_cast<Color::Value>(i);
            Color::Value opponent = Color::Opposite(friendly);
            for (uint8_t j = Piece::PAWN; j < Piece::MAX_ENUM; j++) {
                Piece::Value piece = static_cast<Piece::Value>(j);
                attackers[friendly][piece] = m_AttackTable.GetAttacks(move.to, piece, opponent, pieces.OccupancyMask()) & pieces.OccupancyMask(friendly, piece);
            }
        }

        // Find least valuable attacker
        uint8_t from = UINT8_MAX;
        Piece::Value attacker = Piece::Invalid();
        for (uint8_t i = Piece::PAWN; i < Piece::MAX_ENUM; i++) {
            Bitboard attackerOccupancy = attackers[turn][i];
            if (attackerOccupancy) {
                from = std::countr_zero(attackerOccupancy);
                attacker = static_cast<Piece::Value>(i);
                attackers[turn][i] &= (attackerOccupancy - 1);
                break;
            }
        }
        if (!Piece::IsValid(attacker))
            break;

        // Update gain
        gain.PushBack(Piece::Evaluate(target) - gain[gain.Size() - 1]);

        // Simulate capture
        pieces.Unset(turn, attacker, from);
        pieces.Unset(opponentTurn, target, move.to);
        pieces.Set(turn, attacker, move.to);

        // Swap turn
        target = attacker;
        turn = Color::Opposite(turn);
    }

    // Traverse gain
    for (int8_t i = static_cast<int8_t>(gain.Size()) - 1; i > 0; i--)
        gain[i - 1] = -std::max(-gain[i - 1], gain[i]);

    return gain[0];
}
