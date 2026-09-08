#include "boardState.h"
#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "util/instrumenter.h"

MoveData BoardState::MakeMove(const Zobrist& zobrist, const PieceSquareTables& pst, Move move)
{
    JUPITER_TRACE();

    Color::Value friendly = turn;
    Color::Value enemy = Color::Opposite(turn);
    Piece::Value capture = pieces.PieceInSquare(enemy, move.to);

    // For unmaking the move later
    MoveData moveData = {
        .zobristKey = zobristKey,
        .move = move,
        .capture = capture,
        .rights = rights,
        .turn = turn,
        .enPassantIndex = enPassantIndex,
        .fiftyMoveCounter = fiftyMoveCounter,
        .pstScore = pstScore,
    };

    // Move piece
    pieces.Unset(friendly, move.piece, move.from);
    zobristKey ^= zobrist.ValueForPiece(friendly, move.piece, move.from);
    pstScore -= pst.Get(friendly, move.piece, move.from);
    if (Piece::IsValid(move.promote)) {
        pieces.Set(friendly, move.promote, move.to);
        zobristKey ^= zobrist.ValueForPiece(friendly, move.promote, move.to);
        pstScore += pst.Get(friendly, move.promote, move.to);
    } else {
        pieces.Set(friendly, move.piece, move.to);
        zobristKey ^= zobrist.ValueForPiece(friendly, move.piece, move.to);
        pstScore += pst.Get(friendly, move.piece, move.to);
    }

    // Remove capture
    if (Piece::IsValid(capture)) {
        pieces.Unset(enemy, capture, move.to);
        zobristKey ^= zobrist.ValueForPiece(enemy, capture, move.to);
        pstScore -= pst.Get(enemy, capture, move.to);
    }

    // Move rook if castling
    if (move.piece == Piece::KING && Difference(move.from, move.to) == 2) {
        if (move.from > move.to) {
            pieces.Unset(friendly, Piece::ROOK, move.from - 4);
            zobristKey ^= zobrist.ValueForPiece(friendly, Piece::ROOK, move.from - 4);
            pstScore -= pst.Get(friendly, Piece::ROOK, move.from - 4);

            pieces.Set(friendly, Piece::ROOK, move.from - 1);
            zobristKey ^= zobrist.ValueForPiece(friendly, Piece::ROOK, move.from - 1);
            pstScore += pst.Get(friendly, Piece::ROOK, move.from - 1);
        } else {
            pieces.Unset(friendly, Piece::ROOK, move.from + 3);
            zobristKey ^= zobrist.ValueForPiece(friendly, Piece::ROOK, move.from + 3);
            pstScore -= pst.Get(friendly, Piece::ROOK, move.from + 3);

            pieces.Set(friendly, Piece::ROOK, move.from + 1);
            zobristKey ^= zobrist.ValueForPiece(friendly, Piece::ROOK, move.from + 1);
            pstScore += pst.Get(friendly, Piece::ROOK, move.from + 1);
        }
    }

    // Remove pawn if en passant
    if (move.piece == Piece::PAWN && move.to == enPassantIndex) {
        uint8_t pawnIndex = (friendly == Color::WHITE) ? move.to + 8 : move.to - 8;
        pieces.Unset(enemy, Piece::PAWN, pawnIndex);
        zobristKey ^= zobrist.ValueForPiece(enemy, Piece::PAWN, pawnIndex);
        pstScore -= pst.Get(enemy, Piece::PAWN, pawnIndex);
    }

    // Avoid updating castling rights after both sides lose the right
    if (rights > 0) {
        // Remove castling rights if king moved
        if (move.piece == Piece::KING) {
            zobristKey ^= zobrist.ValueForRights(rights);
            rights = 0;
        }

        // Remove castling rights if rook moved from start square
        if (move.piece == Piece::ROOK) {
            if (move.from == (friendly == Color::WHITE ? 63 : 7)) {
                rights &= ~CastlingRight::Kingside(friendly);
                zobristKey ^= zobrist.ValueForRights(CastlingRight::Kingside(friendly));
            } else if (move.from == (friendly == Color::WHITE ? 56 : 0)) {
                rights &= ~CastlingRight::Queenside(friendly);
                zobristKey ^= zobrist.ValueForRights(CastlingRight::Queenside(friendly));
            }
        }

        // Remove castling rights if rook captured on start square
        if (capture == Piece::ROOK) {
            if (move.to == (enemy == Color::WHITE ? 63 : 7)) {
                rights &= ~CastlingRight::Kingside(enemy);
                zobristKey ^= zobrist.ValueForRights(CastlingRight::Kingside(enemy));
            } else if (move.to == (enemy == Color::WHITE ? 56 : 0)) {
                rights &= ~CastlingRight::Queenside(enemy);
                zobristKey ^= zobrist.ValueForRights(CastlingRight::Queenside(enemy));
            }
        }
    }

    // Update en passant square if double pawn push
    if (enPassantIndex != UINT8_MAX) {
        zobristKey ^= zobrist.ValueForEnPassant(enPassantIndex);
        enPassantIndex = UINT8_MAX;
    }
    if (move.piece == Piece::PAWN && Difference(move.from, move.to) == 16) {
        // Check if any of our pawns can en passant the opponent pawn that just double pushed
        // This means that two identical positions (except for the en passant square) will hash to 
        // the same value as long as there is no pawn to capture en passant.
        // This check only accounts for pseudo-legal en passant captures but is better than nothing.
        uint8_t enPassantIndex = (friendly == Color::WHITE) ? move.to + 8 : move.to - 8;
        uint8_t file = enPassantIndex & 7;
        uint64_t adjacentMask = 0;
        if (file > 0) adjacentMask |= (1ull << (move.to - 1));
        if (file < 7) adjacentMask |= (1ull << (move.to + 1));
        if (adjacentMask & pieces.OccupancyMask(enemy, Piece::PAWN)) {
            zobristKey ^= zobrist.ValueForEnPassant(enPassantIndex);
            this->enPassantIndex = enPassantIndex;
        }
    }

    // Update 50 move counter
    if (Piece::IsValid(capture) || move.piece == Piece::PAWN)
        fiftyMoveCounter = 0;
    else
        fiftyMoveCounter++;

    turn = enemy;
    zobristKey ^= zobrist.ValueForTurn(friendly);
    zobristKey ^= zobrist.ValueForTurn(enemy);

    return moveData;
}

void BoardState::UnmakeMove(MoveData moveData)
{
    JUPITER_TRACE();

    const Move& move = moveData.move;

    Color::Value friendly = moveData.turn;
    Color::Value enemy = Color::Opposite(moveData.turn);

    // Replace moving piece
    pieces.Set(friendly, move.piece, move.from);
    if (Piece::IsValid(move.promote))
        pieces.Unset(friendly, move.promote, move.to);
    else
        pieces.Unset(friendly, move.piece, move.to);

    // Replace capture
    if (Piece::IsValid(moveData.capture))
        pieces.Set(enemy, moveData.capture, move.to);

    // Replace rook if castling
    if (move.piece == Piece::KING && Difference(move.from, move.to) == 2) {
        if (move.from > move.to) {
            pieces.Set(friendly, Piece::ROOK, move.from - 4);
            pieces.Unset(friendly, Piece::ROOK, move.from - 1);
        } else {
            pieces.Set(friendly, Piece::ROOK, move.from + 3);
            pieces.Unset(friendly, Piece::ROOK, move.from + 1);
        }
    }

    // Replace pawn if en passant
    if (move.piece == Piece::PAWN && move.to == moveData.enPassantIndex)
        pieces.Set(enemy, Piece::PAWN, (friendly == Color::WHITE) ? move.to + 8 : move.to - 8);

    // Update variables
    rights = moveData.rights;
    turn = moveData.turn;
    enPassantIndex = moveData.enPassantIndex;
    fiftyMoveCounter = moveData.fiftyMoveCounter;
    zobristKey = moveData.zobristKey;
    pstScore = PSTScore{moveData.pstScore};
}

bool BoardState::WasLegalMove(const AttackTable& attackTable, MoveData moveData)
{
    JUPITER_TRACE();

    Bitboard king = pieces.OccupancyMask(moveData.turn, Piece::KING);

    bool targetAttacked = attackTable.SquareUnderAttack(std::forward<const BoardState>(*this), king, Color::Opposite(moveData.turn));

    // Check intermediate and start squares if castling
    if (moveData.move.piece == Piece::KING && Difference(moveData.move.from, moveData.move.to) == 2) {
        bool startAttacked = attackTable.SquareUnderAttack(std::forward<const BoardState>(*this), 1ull << moveData.move.from, Color::Opposite(moveData.turn));
        uint8_t midIndex = (moveData.move.from > moveData.move.to) ? moveData.move.from - 1 : moveData.move.from + 1;
        bool midAttacked = attackTable.SquareUnderAttack(std::forward<const BoardState>(*this), 1ull << midIndex, Color::Opposite(moveData.turn));

        return !(targetAttacked || startAttacked || midAttacked);
    }

    return !targetAttacked;
}
