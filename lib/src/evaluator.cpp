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
    float phase = GamePhase(std::forward<const BoardState>(state));

    int64_t materialBalance = MaterialBalance(std::forward<const BoardState>(state));
    eval += materialBalance;
    eval += PiecePositions(std::forward<const BoardState>(state), phase);
    eval += Mopup(std::forward<const BoardState>(state), materialBalance, phase);
    eval += CastlingRights(std::forward<const BoardState>(state));

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

int64_t Evaluator::PiecePositions(const BoardState& state, float phase) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    int64_t piecePositionEval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

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

int64_t Evaluator::Mopup(const BoardState& state, int64_t materialBalance, float phase) const 
{
    int64_t mopupEval = 0;

    const int64_t PROXIMITY_FACTOR = 4;
    const int64_t EDGE_FACTOR = 10;

    // Only mopup if up material
    if (materialBalance >= Piece::Evaluate(Piece::PAWN)) {
        uint8_t friendlyKing = std::countr_zero(state.pieces.OccupancyMask(state.turn, Piece::KING));
        uint8_t enemyKing = std::countr_zero(state.pieces.OccupancyMask(Color::Opposite(state.turn), Piece::KING));

        // Bonus for king-king proximity
        mopupEval += (14 - m_DistanceTable.Manhattan(friendlyKing, enemyKing)) * PROXIMITY_FACTOR;

        // Bonus for enemy king proximity to edge
        mopupEval += m_DistanceTable.ManhattanFromCenter(enemyKing) * EDGE_FACTOR;
    }

    // Scale by endgame weight
    return mopupEval * phase;
}

int64_t Evaluator::CastlingRights(const BoardState& state) const 
{
    // Slightly reward having the ability to castle
    int64_t rightsEval = 0;

    const int64_t RIGHTS_BONUS = 10;

    if (state.rights & CastlingRight::Kingside(state.turn))
        rightsEval += RIGHTS_BONUS;

    if (state.rights & CastlingRight::Queenside(state.turn))
        rightsEval += RIGHTS_BONUS;
    
    return rightsEval;
}

int64_t Evaluator::Mobility(const BoardState& state) const 
{
    const int64_t MOBILITY_FACTOR = 10;

    Move killers[2] = { Move::Invalid(), Move::Invalid() };
    Movegen movegen(state, m_AttackTable, killers);

    return (movegen.AttackCount() + movegen.QuietCount()) * MOBILITY_FACTOR;
}

// 0.0 - 1.0 (midgame - endgame)
float Evaluator::GamePhase(const BoardState& state) const
{
    JUPITER_TRACE();
    JUPITER_PROFILE();

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    constexpr int64_t QUEEN_PHASE = 4;
    constexpr int64_t ROOK_PHASE = 2;
    constexpr int64_t BISHOP_PHASE = 1;
    constexpr int64_t KNIGHT_PHASE = 1;

    constexpr int64_t MAX_PHASE = (QUEEN_PHASE * 2) + (ROOK_PHASE * 4) + (BISHOP_PHASE * 4) + (KNIGHT_PHASE * 4);

    int64_t nQueens = state.pieces.Count(friendly, Piece::QUEEN) - state.pieces.Count(enemy, Piece::QUEEN);
    int64_t nRooks = state.pieces.Count(friendly, Piece::ROOK) - state.pieces.Count(enemy, Piece::ROOK);
    int64_t nBishops = state.pieces.Count(friendly, Piece::BISHOP) - state.pieces.Count(enemy, Piece::BISHOP);
    int64_t nKnights = state.pieces.Count(friendly, Piece::KNIGHT) - state.pieces.Count(enemy, Piece::KNIGHT);

    int64_t currentPhase = (QUEEN_PHASE * nQueens) + (ROOK_PHASE * nRooks) + (BISHOP_PHASE * nBishops) + (KNIGHT_PHASE * nKnights);
    currentPhase = std::min(currentPhase, MAX_PHASE); // Clamp in case of promotions

    return 1.0 - (static_cast<float>(currentPhase) / static_cast<float>(MAX_PHASE));
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
