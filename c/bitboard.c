#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "hash.h"


uint_64 SQUARE_MASKS[64];
uint_64 NOT_MASKS[64];

bool const_bitmask_init()
{
    int i;

    for (i=0; i<64; i++) {
        SQUARE_MASKS[i] = 1ul << i;
        NOT_MASKS[i] = ~(SQUARE_MASKS[i]);
    }

    return true;
}


void const_bitmask_verify() {
    int i;

    for (i=0; i<64; i++) {
        printf("Mask[%d] = %lu\nNot Mask[%d] = %lu\n\n",i, SQUARE_MASKS[i], i, NOT_MASKS[i]);
    }
}

void debug_contents_of_bitboard_square(const struct bitChessBoard *pbb, int square)
{
    uint_64 sms = SQUARE_MASKS[square];
    int i;
    bool hit= false;

    printf("Square: %c%c\n", ('a' + square % 8), '1' + (square / 8));
    for (i = 0; i < 15; i++) {
        if (pbb->piece_boards[i] & sms) {
            printf ("\tHit on board %d\n",i);
            hit = true;
        }
    }
    if (!hit) {
        printf("\tEmpty.");
    }

}

struct bitChessBoard *new_bitboard()
{
    struct bitChessBoard *ret;
    ret = (struct bitChessBoard *) malloc (sizeof (bitChessBoard));
    erase_bitboard(ret);
    return ret;
}

bool erase_bitboard(struct bitChessBoard *pbb)
{
    memset(pbb->piece_boards, 0, sizeof(uint_64));
    pbb->ep_target = 0;
    pbb->halfmove_clock = 0;
    pbb->fullmove_number = 1;
    pbb->attrs = 0;
    pbb->halfmoves_completed = 0;
    pbb->hash = 0;
}

int algebraic_to_bitpos(const char alg[2])
{
    return ((uc)((8 * ((alg[1] - '0') + 1)) + 1) + (alg[0] - 'a'));
}


int char_to_piece(char c)
{
    switch (c) {
        case ('p'):
            return BP;
        case ('n'):
            return BN;
        case ('b'):
            return BB;
        case ('r'):
            return BR;
        case ('q'):
            return BQ;
        case ('k'):
            return BK;
        case ('P'):
            return WP;
        case ('N'):
            return WN;
        case ('B'):
            return WB;
        case ('R'):
            return WR;
        case ('Q'):
            return WQ;
        case ('K'):
            return WK;
        default:
            return -1;  // error
    }
}

char bitsquare_to_char(const struct bitChessBoard *pbb, enum boardlayout square)
{
    uint_64 sms;
    sms = SQUARE_MASKS[square];

    if (pbb->piece_boards[ALL_PIECES] & sms) {
        if (pbb->piece_boards[WP] & sms) {
            return 'P';
        } else if (pbb->piece_boards[WN] & sms) {
            return 'N';
        } else if (pbb->piece_boards[WB] & sms){
            return 'B';
        } else if (pbb->piece_boards[WR] & sms) {
            return 'R';
        } else if (pbb->piece_boards[WQ] & sms) {
            return 'Q';
        } else if (pbb->piece_boards[WK] & sms) {
            return 'K';
        } else if (pbb->piece_boards[BP] & sms) {
            return 'p';
        } else if (pbb->piece_boards[BN] & sms) {
            return 'n';
        } else if (pbb->piece_boards[BB] & sms) {
            return 'b';
        } else if (pbb->piece_boards[BR] & sms) {
            return 'r';
        } else if (pbb->piece_boards[BQ] & sms) {
            return 'q';
        } else if (pbb->piece_boards[BK] & sms) {
            return 'k';
        } else {
            assert(false);
            return 'X';
        }
    } else {
        return ' ';
    }
}


bool load_bitboard_from_fen(struct bitChessBoard *pbb, const char *fen) {
    int cur_square = A8;
    char cur_char;
    char ep_pos[2];
    int curpos = 0;
    bool got_wk = false;
    bool got_bk = false;
    int i, piece;
    unsigned int halfmove_clock;
    unsigned int fullmove_number;

    erase_bitboard(pbb);

    cur_char = fen[curpos];
    while (cur_char != '\0' && cur_char != ' ') {

        if (cur_char == '/') {
            // In a proper fen, cur_squre will always be incremented one past the end of the file, or the first square on the next rank
            // so we subtract 2 ranks to get to the next rank to process.
            cur_square = (cur_square - 16);
            if (cur_square % 8 != 0) {
                return false;
            }

        } else if (cur_char >= '1' && cur_char <= '8') {
            cur_square = cur_square + (cur_char - '0');
        } else {
            piece = char_to_piece(cur_char);
            if (piece == -1) {
                return false;
            }
            pbb->piece_boards[piece] |= SQUARE_MASKS[cur_square];
            if (piece == WK) {
                got_wk = true;
            } else if (piece == BK) {
                got_bk = true;
            }
            cur_square++;
        }
        cur_char = fen[++curpos];
    }

    if (!got_wk || !got_bk) {
        return false; // invalid - need to have both kings
    }

    while (cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }

    if (cur_char == 'w') {
        pbb->attrs = pbb->attrs | W_TO_MOVE;
    } else if (cur_char != 'b') {
        return false; // invalid - either white or black must be on move
    }

    cur_char = fen[++curpos];
    while (cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }

    while (cur_char != '\0' && cur_char != ' ') {
        switch (cur_char) {
            case ('K'):
                pbb->attrs = pbb->attrs | W_CASTLE_KING;
                break;
            case ('Q'):
                pbb->attrs = pbb->attrs | W_CASTLE_QUEEN;
                break;
            case ('k'):
                pbb->attrs = pbb->attrs | B_CASTLE_KING;
                break;
            case ('q'):
                pbb->attrs = pbb->attrs | B_CASTLE_QUEEN;
                break;
            case ('-'):
                break;
            default:
                return false; // grammar here is some combination of kqKQ or a - if no castles are possible
        }
        cur_char = fen[++curpos];
    }

    while (cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }
    if (cur_char == '\0') {
        return false; // if we have gotten to end of string and not gotten the remaining fields, it is invalid
    }

    if (cur_char == '-') {
        pbb->ep_target = 0;
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
            pbb->ep_target = algebraic_to_bitpos(ep_pos);
        }
    }

    while (cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }
    if (cur_char == '\0') {
        return false; // we aren't supposed to get to EOL until we have halfmove clock and fullmove number
    }
    i = sscanf(&(fen[curpos]), "%d %d", &halfmove_clock, &fullmove_number);
    if (i < 2) {
        return false; // if we don't get both numbers here, it is invalid.
    }
    pbb->halfmove_clock = halfmove_clock;
    pbb->fullmove_number = fullmove_number;

    if (bitboard_side_to_move_is_in_check(pbb)) {
        pbb->attrs = pbb->attrs | BOARD_IN_CHECK;
    }

    pbb->hash = compute_bitboard_hash(pbb);

    pbb->piece_boards[WHITE] = pbb->piece_boards[WP] | pbb->piece_boards[WN] | pbb->piece_boards[WB] | pbb->piece_boards[WR] | pbb->piece_boards[WQ] | pbb->piece_boards[WK];
    pbb->piece_boards[BLACK] = pbb->piece_boards[BP] | pbb->piece_boards[BN] | pbb->piece_boards[BB] | pbb->piece_boards[BR] | pbb->piece_boards[BQ] | pbb->piece_boards[BK];
    pbb->piece_boards[ALL_PIECES] = pbb->piece_boards[WHITE] | pbb->piece_boards[BLACK];

    return true;
}

char *convert_bitboard_to_fen(const struct bitChessBoard *pbb)
{
    char *ret;
    int rank, file, i;
    int curoccupant;
    int num_blanks;
    int curchar = 0;
    bool hascastle = false;
    char fen_ending[15];  // way much more than we need.

    ret = (char *) malloc (256 * sizeof(char));  // way much more than we need.  64 squares + 7 rank separators + 4 castle + 2 for EP target = 87.  Even with the ending and spaces this is huge.
    for (rank = A8; rank >= 0; rank -= 8) {
        num_blanks = 0;
        for (file = 0; file < 8; file++) {
            if (!(pbb->piece_boards[ALL_PIECES] & SQUARE_MASKS[rank+file])) {
                num_blanks ++;
            }
            else {
                if (num_blanks > 0) {
                    ret[curchar++] = ('0' + num_blanks);
                    num_blanks = 0;
                }
                ret [curchar++] = bitsquare_to_char(pbb, rank+file);
            }
        }
        if (num_blanks > 0) {
            ret[curchar++] = ('0' + num_blanks);
        }
        if (rank > 1) {
            ret[curchar++] = '/';
        }
    }
    ret[curchar++] = ' ';
    if (pbb->attrs & W_TO_MOVE) {
        ret[curchar++] = 'w';
    } else {
        ret[curchar++] = 'b';
    }
    ret[curchar++] = ' ';
    if (pbb->attrs & W_CASTLE_KING) {
        ret[curchar++] = 'K';
        hascastle = true;
    }
    if (pbb->attrs & W_CASTLE_QUEEN) {
        ret[curchar++] = 'Q';
        hascastle = true;
    }
    if (pbb->attrs & B_CASTLE_KING) {
        ret[curchar++] = 'k';
        hascastle = true;
    }
    if (pbb->attrs & B_CASTLE_QUEEN) {
        ret[curchar++] = 'q';
        hascastle = true;
    }
    if (!hascastle) {
        ret[curchar++] = '-';
    }
    ret[curchar++] = ' ';
    if (pbb->ep_target == 0) {
        ret[curchar++] = '-';
    } else {
        ret[curchar++] = ('a' + (pbb->ep_target % 8));
        ret[curchar++] = ('1' + (pbb->ep_target / 8));
    }
    ret[curchar++] = ' ';

    snprintf(fen_ending, 15, "%d %d", pbb->halfmove_clock, pbb->fullmove_number);
    // quick hack that is safe because ret is about 2x as long as we need for this:
    for (i=0; i < 15; i++) {
        ret[curchar++] = fen_ending[i];
    }
    ret [curchar] = '\0'; // likely not needed becuase fen_ending is null terminated, but safer.
    return ret;
}

bool set_bitboard_startpos(struct bitChessBoard *pbb)
{
    load_bitboard_from_fen(pbb, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w kqKQ - 0 1");
}


bool bitboard_side_to_move_is_in_check(const struct bitChessBoard *pbb)
{
    // stub
    return false;

}