#include <cstdint>
#include "movegen/attackTable.h"
#include "movegen/move.h"
#include "board/boardState.h"
#include "core.h"

const uint64_t MAX_POSSIBLE_QUIETS = 100;
const uint64_t MAX_POSSIBLE_ATTACKS = 100;

using QuietMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS>;
using AttackMoveBuffer = Buffer<Move, MAX_POSSIBLE_ATTACKS>;
using CombinedMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS + MAX_POSSIBLE_ATTACKS>;

class Movegen {
public:
    Movegen(const BoardState& state, const AttackTable& attackTable)
        : m_State{std::forward<const BoardState>(state)}, m_AttackTable{std::forward<const AttackTable>(attackTable)} {}

    void FindAllAttacks(AttackMoveBuffer& attacks);
    void FindAllQuiets(QuietMoveBuffer& quiets);
    void FindPawnAttacks(uint8_t index, AttackMoveBuffer& attacks);
    void FindPawnQuiets(uint8_t index, QuietMoveBuffer& quiets); 
    void FindKnightAttacks(uint8_t index, AttackMoveBuffer& attacks);
    void FindKnightQuiets(uint8_t index, QuietMoveBuffer& quiets);
    void FindKingAttacks(uint8_t index, AttackMoveBuffer& attacks);
    void FindKingQuiets(uint8_t index, QuietMoveBuffer& quiets);
    void FindBishopAttacks(uint8_t index, AttackMoveBuffer& attacks);
    void FindBishopQuiets(uint8_t index, QuietMoveBuffer& quiets);
    void FindRookAttacks(uint8_t index, AttackMoveBuffer& attacks);
    void FindRookQuiets(uint8_t index, QuietMoveBuffer& quiets);
    void FindQueenAttacks(uint8_t index, AttackMoveBuffer& attacks);
    void FindQueenQuiets(uint8_t index, QuietMoveBuffer& quiets);
    void FindSliderAttacks(uint8_t index, Piece::Value piece, AttackMoveBuffer& attacks);
    void FindSliderQuiets(uint8_t index, Piece::Value piece, QuietMoveBuffer& quiets);

private:
    const BoardState& m_State;
    const AttackTable& m_AttackTable;
};
