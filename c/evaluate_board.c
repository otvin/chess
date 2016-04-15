#include "evaluate_board.h"


int piece_value(uc piece)
{
    if (piece & PAWN) {
        return PAWN_VALUE;
    } else if (piece & KNIGHT) {
        return KNIGHT_VALUE;
    } else if (piece & BISHOP) {
        return BISHOP_VALUE;
    } else if (piece & ROOK) {
        return ROOK_VALUE;
    } else if (piece & QUEEN) {
        return QUEEN_VALUE;
    } else if (piece & KING) {
        return KING_VALUE;
    }
    return 0;
}