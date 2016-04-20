#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#include "chess_constants.h"
#include "chessmove.h"



Move create_move(uc start, uc end, uc piece_moving, uc piece_captured, uc promoted_to, uc move_flags)
{
    Move ret = 0;

    ret = ret | start;
    ret = ret | (Move) end << END_SHIFT;
    ret = ret | (Move) piece_moving << PIECE_MOVING_SHIFT;
    ret = ret | (Move) piece_captured << PIECE_CAPTURED_SHIFT;
    ret = ret | (Move) promoted_to << PROMOTED_TO_SHIFT;
    ret = ret | (Move) move_flags << MOVE_FLAGS_SHIFT;

    return (ret);
}

bool parse_move(Move move, uc *pStart, uc *pEnd, uc *pPiece_moving, uc *pPiece_captured, uc *pPromoted_to, uc *pMove_flags)
{
    *pStart = (uc) (move & START);
    *pEnd = (uc) ((move & END) >> END_SHIFT);
    *pPiece_moving = (uc) ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT);
    *pPiece_captured = (uc) ((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT);
    *pPromoted_to = (uc) ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT);
    *pMove_flags = (uc) ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT);

    return true;
}

char *pretty_print_move(Move move) {

    uc start, end;
    uc piece_moving, promoted_to, flags, piece_captured;
    char startrank;
    char startfile;
    char endrank;
    char endfile;
    char movechar;
    char checkchar;
    char promotion_and_check[5];

    char *ret;

    ret = (char *)malloc(10 * sizeof(char));  /* Largest move size is 9 characters:  a7-a8(Q)+ */
    memset(ret, '\0', sizeof(ret));

    if (!parse_move(move, &start, &end, &piece_moving, &piece_captured, &promoted_to, &flags)) {
        return(ret);
    }

    /* These can be used later for SAN notation and for debugging:
     *     piece_moving = (uc) ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT);
     *     capture_differential = (int) (((move & CAPTURE_DIFFERENTIAL) >> CAPTURE_DIFFERENTIAL_SHIFT) - CAPTURE_DIFFERENTIAL_OFFSET);
     */

    if (move == NULL_MOVE) {
        snprintf(ret, 10, "{END}");
        return ret;
    } else {
        if (flags & MOVE_CHECK) {
            checkchar = '+';
        } else {
            checkchar = '\0';
        }
        if (flags & MOVE_CASTLE) {

            if (end > start) {
                snprintf(ret, 10, "O-O%c", checkchar);
            } else {
                snprintf(ret, 10, "O-O-O%c", checkchar);
            }
        } else {
            startrank = 48 + (start / 10) - 1; //48 = '0';
            startfile = 97 + (start % 10) - 1; //97 = 'a';
            endrank = 48 + (end / 10) - 1;
            endfile = 97 + (end % 10) - 1;
            if (move & PIECE_CAPTURED) {
                movechar = 'x';
            } else {
                movechar = '-';
            }
            switch(promoted_to){
                case (WN):
                case (BN):
                    snprintf(promotion_and_check, 5, "(N)%c", checkchar);
                    break;
                case (WB):
                case (BB):
                    snprintf(promotion_and_check, 5, "(B)%c", checkchar);
                    break;
                case (WR):
                case (BR):
                    snprintf(promotion_and_check, 5, "(R)%c", checkchar);
                    break;
                case (WQ):
                case (BQ):
                    snprintf(promotion_and_check, 5, "(Q)%c", checkchar);
                    break;
                default:
                    snprintf(promotion_and_check, 5, "%c", checkchar);

            }
            snprintf(ret, 10, "%c%c%c%c%c%s", startfile, startrank, movechar, endfile, endrank, promotion_and_check);
        }
    }
    return (ret);
}

