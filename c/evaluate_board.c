#include "evaluate_board.h"


int piece_value(uc piece)
{
    // pieces are stored in the last three bits
    uc p = (piece & 7);

    switch(p) {
        case (PAWN):
            return PAWN_VALUE;
        case (KNIGHT):
            return KNIGHT_VALUE;
        case (BISHOP):
            return BISHOP_VALUE;
        case (ROOK):
            return ROOK_VALUE;
        case (QUEEN):
            return QUEEN_VALUE;
        case (KING):
            return KING_VALUE;
    }

    return 0;
}