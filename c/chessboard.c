#include <stdlib.h>
#include <stdio.h>

#include "chess_constants.h"
#include "chessmove.h"
#include "check_tables.h"
#include "chessboard.h"
#include "hash.h"


bool arraypos_is_on_board(uc pos)
{
    /*
    layout of the board - count this way from 0..119

    110 111 112 113 114 115 116 117 118 119
    100 101 102 103 104 105 106 107 108 109
    ...
    10 11 12 13 14 15 16 17 18 19
    00 01 02 03 04 05 06 07 08 09

    squares 21-98, except for squares with 0 or 9 in the 1's space are the board.  Everything else is off board.

    */
    bool ret;
    uc ones_digit;

    ret = true;
    if (pos < 21 || pos > 98) {
        ret = false;
    }
    else {
        ones_digit = pos % 10;
        if (ones_digit == 0 || ones_digit == 9) {
            ret = false;
        }
    }
    return (ret);
}

uc algebraic_to_arraypos(char alg[2])
{
    return ((uc)((10 * ((alg[1] - '0') + 1)) + 1) + (alg[0] - 'a'));
}

char square_to_charpiece(uc s)
{
    switch(s){
        case(WP):
            return('P');
        case(WN):
            return('N');
        case(WB):
            return('B');
        case(WR):
            return('R');
        case(WQ):
            return('Q');
        case(WK):
            return('K');
        case(BP):
            return('p');
        case(BN):
            return('n');
        case(BB):
            return('b');
        case(BR):
            return('r');
        case(BQ):
            return('q');
        case(BK):
            return('k');
        case(EMPTY):
            return('.');
        default:
            return('X');
    }
}

uc charpiece_to_square(char piece)
{
    switch (piece) {
        case('p'):
            return BP;
        case('n'):
            return BN;
        case('b'):
            return BB;
        case('r'):
            return BR;
        case('q'):
            return BQ;
        case('k'):
            return BK;
        case('P'):
            return WP;
        case('N'):
            return WN;
        case('B'):
            return WB;
        case('R'):
            return WR;
        case('Q'):
            return WQ;
        case('K'):
            return WK;
        case(' '):
        case('.'):
            return EMPTY;
        case('X'):
        default:
            return OFF_BOARD;
    }
}

void erase_board(struct ChessBoard *pb)
{
    uc i;

    for (i=0; i<120; i++) {
        if (arraypos_is_on_board(i)) {
            pb->squares[i] = EMPTY;
        }
        else {
            pb->squares[i] = OFF_BOARD;
        }
    }
    pb->ep_target = 0;
    pb->halfmove_clock = 0;
    pb->fullmove_number = 1;
    pb->attrs = 0;
    pb->halfmoves_completed = 0;
    pb->hash = 0;
}

void set_start_position(struct ChessBoard *pb)
{
    load_from_fen(pb, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w kqKQ - 0 1");
}


bool load_from_fen(struct ChessBoard *pb, const char *fen)
{
    uc cur_square = 91;
    char cur_char;
    char ep_pos[2];
    int curpos = 0;
    bool got_wk = false;
    bool got_bk = false;
    int i;
    unsigned int halfmove_clock;
    unsigned int fullmove_number;

    erase_board(pb);

    cur_char = fen[curpos];
    while (cur_char != '\0' && cur_char != ' ') {

        if (cur_char == '/') {
            cur_square = (10 * ((cur_square - 10) / 10)) + 1; // move to first square in next rank
        } else if (cur_char >= '1' && cur_char <= '8') {
            cur_square = cur_square + (cur_char - '0');
        } else {
            pb->squares[cur_square] = charpiece_to_square(cur_char);
            if (pb->squares[cur_square] == WK) {
                got_wk = true;
            } else if (pb->squares[cur_square] == BK) {
                got_bk = true;
            }
            cur_square++;
        }
        cur_char = fen[++curpos];
    }

    if (!got_wk || !got_bk) {
        return false; // invalid - need to have both kings
    }

    while(cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }

    if (cur_char == 'w') {
        pb->attrs = pb->attrs | W_TO_MOVE;
    } else if (cur_char != 'b') {
        return false; // invalid - either white or black must be on move
    }

    cur_char = fen[++curpos];
    while(cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }

    while(cur_char != '\0' && cur_char != ' ') {
        switch(cur_char) {
            case('K'):
                pb->attrs = pb->attrs | W_CASTLE_KING;
                break;
            case('Q'):
                pb->attrs = pb->attrs | W_CASTLE_QUEEN;
                break;
            case('k'):
                pb->attrs = pb->attrs | B_CASTLE_KING;
                break;
            case('q'):
                pb->attrs = pb->attrs | B_CASTLE_QUEEN;
                break;
            case('-'):
                break;
            default:
                return false; // grammar here is some combination of kqKQ or a - if no castles are possible
        }
        cur_char = fen[++curpos];
    }

    while(cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }
    if (cur_char == '\0') {
        return false; // if we have gotten to end of string and not gotten the remaining fields, it is invalid
    }

    if (cur_char == '-') {
        pb->ep_target = 0;
        cur_char = fen[++curpos];
    } else {
        // validate input
        ep_pos[0] = cur_char;
        cur_char = fen[++curpos];
        if (cur_char == '\0') {
            return false; // if we didn't get a hyphen here, we need a 2-character algebraic position, so shouldn't be at end of string
        } else {
            ep_pos[1] = cur_char;
            cur_char = fen[++curpos];
            pb->ep_target = algebraic_to_arraypos(ep_pos);
            // cur_char = fen[++curpos];
        }
    }

    while(cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }
    if (cur_char == '\0') {
        return false; // we aren't supposed to get to EOL until we have halfmove clock and fullmove number
    }
    i = sscanf(&(fen[curpos]), "%d %d", &halfmove_clock, &fullmove_number);
    if (i < 2) {
        return false; // if we don't get both numbers here, it is invalid.
    }
    pb->halfmove_clock = halfmove_clock;
    pb->fullmove_number = fullmove_number;

    if (side_to_move_is_in_check(pb)) {
        pb->attrs = pb->attrs | BOARD_IN_CHECK;
    }

    pb->hash = compute_hash(pb);

    return true;

}


char *print_board(const struct ChessBoard *pb)
{
    int i;
    int j;
    int pos=0;
    char *ret;
    ret = (char *)malloc(73 * sizeof(char)); // 8x8 = 64, + 8 carriage returns = 72, + '\0'.
    ret[72] = '\0';

    for (i = 90; i > 10; i = i-10) {
        for (j = 1; j < 9; j++) {
            ret[pos++] = square_to_charpiece(pb->squares[i+j]);
        }
        ret[pos++] = '\n';
    }
    return ret;
}

void debugprint_move_history(const struct ChessBoard *pb) {
    int i;
    char *moveprint;

    for (i=0; i< pb->halfmoves_completed; i++) {
        moveprint = pretty_print_move(pb->move_history[i]);
        if ((i+1) % 2 == 1) {
            printf("%d. %s", (i/2) + 1, moveprint);
        } else {
            printf(" %s\n", moveprint);
        }
        free(moveprint);
    }
    printf("\n\n");
}

struct ChessBoard *new_board()
{
    struct ChessBoard *ret;
    ret = (ChessBoard *)malloc(sizeof(ChessBoard));
    return(ret);
}

void apply_move(struct ChessBoard *pb, Move m) {
    // no attempt to validate the move, just apply it

    uc start, end;
    uc piece_moving, piece_captured, promoted_to, move_flags, oldattrs, attrdiffs;

    start = GET_START(m);
    end = GET_END(m);
    piece_captured = GET_PIECE_CAPTURED(m);
    promoted_to = GET_PROMOTED_TO(m);
    piece_moving = GET_PIECE_MOVING(m);
    move_flags = GET_FLAGS(m);


    pb->squares[start] = EMPTY;
    pb->hash ^= hashsquare_for_bitflag_piece(piece_moving, start);

    if (promoted_to) {
        pb->squares[end] = promoted_to;
        pb->hash ^= hashsquare_for_bitflag_piece(promoted_to, end);
    } else {
        pb->squares[end] = piece_moving;
        pb-> hash ^= hashsquare_for_bitflag_piece(piece_moving, end);
    }

    if (piece_captured) {
        if (move_flags & MOVE_EN_PASSANT) {
            if (piece_moving & BLACK) {
                pb->squares[end + 10] = EMPTY;
                pb->hash ^= hashsquare_for_bitflag_piece(WP, end+10);
            } else {
                pb->squares[end - 10] = EMPTY;
                pb->hash ^= hashsquare_for_bitflag_piece(BP, end-10);
            }
        } else {
            pb->hash ^= hashsquare_for_bitflag_piece(piece_captured, end);
        }
    }

    if (pb->ep_target > 0) {
        pb->hash ^= hash_enpassanttarget[pb->ep_target];
    }

    if (move_flags & MOVE_DOUBLE_PAWN) {
        if (piece_moving & BLACK) {
            pb->ep_target = end + 10;
            pb->hash ^= hash_enpassanttarget[end+10];
        } else {
            pb->ep_target = end - 10;
            pb->hash ^= hash_enpassanttarget[end-10];
        }
    } else {
        pb->ep_target = 0;

        if (move_flags & MOVE_CASTLE) {
            switch (end) {
                case (27):
                    pb->squares[28] = EMPTY;
                    pb->squares[26] = WR;
                    pb->hash ^= hash_whiter[28];
                    pb->hash ^= hash_whiter[26];
                    pb->hash ^= hash_whitecastleking;
                    if (pb->attrs & W_CASTLE_QUEEN) {
                        pb->hash ^= hash_whitecastlequeen;
                    }
                    pb->attrs = pb->attrs & ~(W_CASTLE_KING | W_CASTLE_QUEEN);
                    break;
                case (23):
                    pb->squares[21] = EMPTY;
                    pb->squares[24] = WR;
                    pb->hash ^= hash_whiter[21];
                    pb->hash ^= hash_whiter[24];
                    pb->hash ^= hash_whitecastlequeen;
                    if (pb->attrs & W_CASTLE_KING) {
                        pb->hash ^= hash_whitecastleking;
                    }
                    pb->attrs = pb->attrs & ~(W_CASTLE_KING | W_CASTLE_QUEEN);
                    break;
                case (97):
                    pb->squares[98] = EMPTY;
                    pb->squares[96] = BR;
                    pb->hash ^= hash_blackr[98];
                    pb->hash ^= hash_blackr[96];
                    pb->hash ^= hash_blackcastleking;
                    if (pb->attrs & B_CASTLE_QUEEN) {
                        pb->hash ^= hash_blackcastlequeen;
                    }
                    pb->attrs = pb->attrs & ~(B_CASTLE_KING | B_CASTLE_QUEEN);
                    break;
                case (93):
                    pb->squares[91] = EMPTY;
                    pb->squares[94] = BR;
                    pb->hash ^= hash_blackr[91];
                    pb->hash ^= hash_blackr[94];
                    pb->hash ^= hash_blackcastlequeen;
                    if (pb->attrs & B_CASTLE_KING) {
                        pb->hash ^= hash_blackcastleking;
                    }
                    pb->attrs = pb->attrs & ~(B_CASTLE_KING | B_CASTLE_QUEEN);
                    break;
            }
        } else {
            oldattrs = pb->attrs;
            if (pb->attrs & (W_CASTLE_KING | W_CASTLE_QUEEN)) {
                if (piece_moving == WK) {
                    pb->attrs = pb->attrs & ~(W_CASTLE_KING | W_CASTLE_QUEEN);
                } else if (piece_moving == WR) {
                    if (start == 21) {
                        pb->attrs = pb->attrs & (~W_CASTLE_QUEEN);
                    } else if (start == 28) {
                        pb->attrs = pb->attrs & (~W_CASTLE_KING);
                    }
                }
            }
            if (pb->attrs & (B_CASTLE_KING | B_CASTLE_QUEEN)) {
                if (piece_moving == BK) {
                    pb->attrs = pb->attrs & ~(B_CASTLE_KING | B_CASTLE_QUEEN);
                } else if (piece_moving == BR) {
                    if (start == 91) {
                        pb->attrs = pb->attrs & (~B_CASTLE_QUEEN);
                    } else if (start == 98) {
                        pb->attrs = pb->attrs & (~B_CASTLE_KING);
                    }
                }
            }
            attrdiffs = oldattrs ^ pb->attrs;
            if (attrdiffs) {
                if (attrdiffs & W_CASTLE_KING) {
                    pb->hash ^= hash_whitecastleking;
                }
                if (attrdiffs & W_CASTLE_QUEEN) {
                    pb->hash ^= hash_whitecastlequeen;
                }
                if (attrdiffs & B_CASTLE_KING) {
                    pb->hash ^= hash_blackcastleking;
                }
                if (attrdiffs & B_CASTLE_QUEEN) {
                    pb->hash ^= hash_blackcastlequeen;
                }
            }
        }
    }

    if (move_flags & MOVE_CHECK) {
        pb -> attrs = pb -> attrs | BOARD_IN_CHECK;
    } else {
        pb -> attrs = pb -> attrs & (~BOARD_IN_CHECK);
    }

    if ((piece_moving & PAWN) || piece_captured) {
        pb -> halfmove_clock = 0;
    } else {
        pb -> halfmove_clock ++;
    }

    if (!(pb->attrs & W_TO_MOVE)) {
        pb -> fullmove_number ++;
        pb->attrs = pb->attrs | W_TO_MOVE;
    } else {
        pb->attrs = pb->attrs & (~W_TO_MOVE);
    }

    pb->hash ^= hash_whitetomove;

    pb->move_history[(pb->halfmoves_completed)++] = m;
    if (pb->halfmoves_completed >= MAX_MOVE_HISTORY) {
        // TODO - something better here?  At least this won't coredump.  But we lose history after 128 moves.
        pb->halfmoves_completed = 0;
    }
}

uc find_next_piece_location(const struct ChessBoard *pb, uc piece, uc index)
{
    uc i;

    if (index < 21) {
        index = 21;  // allows people to pass in 0 for the start, but 0 is offboard.  21 is smallest on-board index.
    }

    for (i = index; i < 120; i++) {
           if (pb->squares[i] == piece) {
            return i;
        }
    }
    return 0;
}

bool side_to_move_is_in_check(const struct ChessBoard *pb) {

    uc curpos = 0, occupant;
    uc kingpos, square_to_check;

    if (pb->attrs & W_TO_MOVE) {
        /* The white and black loops are the same except for the array being tested and the test for piece
         * of enemy color.  There may be a better way to do this, but this way does at least save one extra
         * comparison per loop. */
        kingpos = find_next_piece_location(pb, WK, 0);
        if (kingpos == 0) {
            return false;
        }
        square_to_check = WHITE_CHECK_TABLE[kingpos][curpos][0];
        while (square_to_check != 0) {
            occupant = pb->squares[square_to_check];
            if (occupant & WHITE_CHECK_TABLE[kingpos][curpos][1]) {
                if (occupant & BLACK) {
                    return true;
                } else {
                    curpos = WHITE_CHECK_TABLE[kingpos][curpos][2];  // this direction is blocked
                }
            } else if (occupant) {
                curpos = WHITE_CHECK_TABLE[kingpos][curpos][2];
            } else {
                curpos ++;
            }
            square_to_check = WHITE_CHECK_TABLE[kingpos][curpos][0];
        }
    } else {
        kingpos = find_next_piece_location(pb, BK, 0);
        if (kingpos == 0) {
            return false;
        }
        square_to_check = BLACK_CHECK_TABLE[kingpos][curpos][0];
        while (square_to_check != 0) {
            occupant = pb->squares[square_to_check];
            if (occupant & BLACK_CHECK_TABLE[kingpos][curpos][1]) {
                if (!(occupant & BLACK)) {
                    return true;
                } else {
                    curpos = BLACK_CHECK_TABLE[kingpos][curpos][2];
                }
            } else if (occupant) {
                curpos = BLACK_CHECK_TABLE[kingpos][curpos][2];
            } else {
                curpos ++;
            }
            square_to_check = BLACK_CHECK_TABLE[kingpos][curpos][0];
        }
    }

    return false;
}