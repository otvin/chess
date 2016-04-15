#include <stdlib.h>
#include <stdio.h>

#include "chess_constants.h"
#include "chessmove.h"
#include "check_tables.h"
#include "chessboard.h"


bool arraypos_is_on_board(square pos)
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
    square ones_digit;

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

char square_to_charpiece(square s)
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

square charpiece_to_square(char piece)
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
    square i;

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
}

void set_start_position(struct ChessBoard *pb)
{
    int i;

    erase_board(pb);
    for (i=0; i<=120; i++) {
        if (31 <= i && i <= 38)
            pb->squares[i] = WP;
        else if (81 <= i && i <= 88)
            pb->squares[i] = BP;
        else if (i == 21 || i == 28)
            pb->squares[i] = WR;
        else if (i == 22 || i == 27)
            pb->squares[i] = WN;
        else if (i == 23 || i == 26)
            pb->squares[i] = WB;
        else if (i == 24)
            pb->squares[i] = WQ;
        else if (i == 25)
            pb->squares[i] = WK;
        else if (i == 91 || i == 98)
            pb->squares[i] = BR;
        else if (i == 92 || i == 97)
            pb->squares[i] = BN;
        else if (i == 93 || i == 96)
            pb->squares[i] = BB;
        else if (i == 94)
            pb->squares[i] = BQ;
        else if (i == 95)
            pb->squares[i] = BK;
    }
    pb->ep_target = 0;
    pb->attrs = pb->attrs | (W_TO_MOVE | W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN);
}


bool load_from_fen(struct ChessBoard *pb, const char *fen)
{
    uc cur_square = 91;
    char cur_char;
    char ep_pos[2];
    short curpos = 0;
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


struct ChessBoard *new_board()
{
    struct ChessBoard *ret;
    ret = (ChessBoard *)malloc(sizeof(ChessBoard));
    return(ret);
}

void apply_move(struct ChessBoard *pb, Move m) {
    // no attempt to validate the move, just apply it

    square start, end;
    uc piece_moving, piece_captured, promoted_to, move_flags;
    short capture_diff;

    parse_move(m, &start, &end, &piece_moving, &piece_captured, &capture_diff, &promoted_to, &move_flags);

    pb->squares[start] = EMPTY;
    if (promoted_to) {
        pb->squares[end] = promoted_to;
    } else {
        pb->squares[end] = piece_moving;
    }

    if (move_flags & MOVE_EN_PASSANT) {
        if (piece_moving & BLACK) {
            pb->squares[end-10] = EMPTY;
        } else {
            pb->squares[end+10] = EMPTY;
        }
    }

    if (move_flags & MOVE_DOUBLE_PAWN) {
        if (piece_moving & BLACK) {
            pb->ep_target = end + 10;
        } else {
            pb->ep_target = end - 10;
        }
    } else {
        pb->ep_target = 0;

        if (move_flags & MOVE_CASTLE) {
            switch (end) {
                case (27):
                    pb->squares[28] = EMPTY;
                    pb->squares[26] = WR;
                    pb->attrs = pb->attrs & ~(W_CASTLE_KING | W_CASTLE_QUEEN);
                    break;
                case (23):
                    pb->squares[21] = EMPTY;
                    pb->squares[24] = WR;
                    pb->attrs = pb->attrs & ~(W_CASTLE_KING | W_CASTLE_QUEEN);
                    break;
                case (97):
                    pb->squares[98] = EMPTY;
                    pb->squares[96] = BR;
                    pb->attrs = pb->attrs & ~(B_CASTLE_KING | B_CASTLE_QUEEN);
                    break;
                case (93):
                    pb->squares[91] = EMPTY;
                    pb->squares[94] = BR;
                    pb->attrs = pb->attrs & ~(B_CASTLE_KING | B_CASTLE_QUEEN);
                    break;
            }
        } else {
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
}

square find_next_piece_location(const struct ChessBoard *pb, uc piece, uc index)
{
    square i;

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
    square kingpos, square_to_check;

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