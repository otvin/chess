#include <memory.h>
#include "chessmove.h"



Move create_move(square start, square end, uc piece_moving, uc piece_captured, short capture_differential, uc promoted_to, uc move_flags) {
    Move ret = 0;

    ret = ret | start;
    ret = ret | (Move) end << END_SHIFT;
    ret = ret | (Move) piece_moving << PIECE_MOVING_SHIFT;
    ret = ret | (Move) piece_captured << PIECE_CAPTURED_SHIFT;
    ret = ret | (Move) capture_differential << CAPTURE_DIFFERENTIAL_SHIFT;
    ret = ret | (Move) promoted_to << PROMOTED_TO_SHIFT;
    ret = ret | (Move) move_flags << MOVE_FLAGS_SHIFT;

    return (ret);
}

char *pretty_print_move(Move move) {

    square start, end;
    uc piece_moving, piece_captured, promoted_to, flags;
    short capture_differential;
    char startrank;
    char startfile;
    char endrank;
    char endfile;
    char movechar;
    char checkchar;
    char promotion[4];

    char *ret;

    ret = (char *)malloc(10 * sizeof(char));  /* Largest move size is 9 characters:  a7-a8(Q)+ */
    memset(ret, '\0', sizeof(ret));

    /* These can be used later for SAN notation and for debugging:
     *     piece_moving = (uc) ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT);
     *     capture_differential = (short) ((move & CAPTURE_DIFFERENTIAL) >> CAPTURE_DIFFERENTIAL_SHIFT);
     */

    if (move == NULL_MOVE) {
        snprintf(ret, 10, "{END}");
        return ret;
    } else {
        start = (square) (move & START);
        end = (square) ((move & END) >> END_SHIFT);
        flags = (uc) ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT);

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
            promoted_to = (uc) ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT);

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
                    strcpy(promotion, "(N)\0");
                    break;
                case (WB):
                case (BB):
                    strcpy(promotion,"(B)\0");
                    break;
                case (WR):
                case (BR):
                    strcpy(promotion,"(R)\0");
                    break;
                case (WQ):
                case (BQ):
                    strcpy(promotion,"(Q)\0");
                    break;
                default:
                    memset(promotion, '\0', sizeof(promotion));

            }
            snprintf(ret, 10, "%c%c%c%c%c%s", startfile, startrank, movechar, endfile, endrank, promotion);
        }
    }
    return (ret);
}

struct MoveList *new_empty_move_list() {
    struct MoveList *r;

    r = (struct MoveList *) malloc (sizeof(struct MoveList));
    r -> first = NULL;
    r -> last = NULL;

    return (r);

}

void add_move_to_list(struct MoveList *pList, Move move) {
    struct MoveListNode *n;
    struct MoveListNode *p;

    n = (struct MoveListNode *) malloc (sizeof(struct MoveListNode));
    n->m = move;
    n->next = NULL;

    if (pList->first == NULL) {
        pList->first = n;
    }
    else {
        p = pList->last;
        p -> next = n;
    }
    pList->last = n;
}

void delete_moves_in_list(struct MoveList *pList) {

    struct MoveListNode *cur;
    struct MoveListNode *next;

    cur = pList->first;
    while (cur != NULL) {
        next = cur->next;
        free(cur);
        cur = next;
    }
}


void print_move_list(struct MoveList list) {
    struct MoveListNode *p;
    char *movestr;
    p = list.first;
    while (p != NULL) {
        movestr = pretty_print_move(p->m);
        printf("%s\n", movestr);
        free(movestr);
        p = p->next;
    }
}
