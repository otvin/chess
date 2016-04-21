#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "hash.h"


uint_64 SQUARE_MASKS[64];
uint_64 NOT_MASKS[64];

uint_64 A_FILE;
uint_64 H_FILE;
uint_64 RANK_1;
uint_64 RANK_8;
uint_64 NOT_A_FILE;
uint_64 NOT_H_FILE;
uint_64 NOT_RANK_1;
uint_64 NOT_RANK_8;

uint_64 B_FILE;
uint_64 G_FILE;
uint_64 RANK_2;
uint_64 RANK_7;
uint_64 NOT_B_FILE;
uint_64 NOT_G_FILE;
uint_64 NOT_RANK_2;
uint_64 NOT_RANK_7;


uint_64 KNIGHT_MOVES[64];
uint_64 KING_MOVES[64];

int pop_lsb(uint_64 *i)
{
    // returns 0-63 which would be the position of the first 1, unless 0 is passed in, in which case
    // it returns -1.  ctzll() would return 64, but that would case the NOT_MASKS lookup to barf.  But avoiding the
    // special case for 0 would make things go faster.
    if (*i == 0) {
        return -1;
    }

    // TODO - see if there is a way to optimize this as it will be done a ton.
    int lsb;
    lsb = __builtin_ctzll(*i);
    *i = *i & NOT_MASKS[lsb];
    return lsb;
}

bool const_bitmask_init()
{
    int i;
    uint_64 cursquare;

    for (i=0; i<64; i++) {
        SQUARE_MASKS[i] = 1ul << i;
        NOT_MASKS[i] = ~(SQUARE_MASKS[i]);
    }

    A_FILE = SQUARE_MASKS[A1] | SQUARE_MASKS[A2] | SQUARE_MASKS[A3] | SQUARE_MASKS[A4] | SQUARE_MASKS[A5] | SQUARE_MASKS[A6] | SQUARE_MASKS[A7] | SQUARE_MASKS[A8];
    H_FILE = SQUARE_MASKS[H1] | SQUARE_MASKS[H2] | SQUARE_MASKS[H3] | SQUARE_MASKS[H4] | SQUARE_MASKS[H5] | SQUARE_MASKS[H6] | SQUARE_MASKS[H7] | SQUARE_MASKS[H8];
    RANK_1 = SQUARE_MASKS[A1] | SQUARE_MASKS[B1] | SQUARE_MASKS[C1] | SQUARE_MASKS[D1] | SQUARE_MASKS[E1] | SQUARE_MASKS[F1] | SQUARE_MASKS[G1] | SQUARE_MASKS[H1];
    RANK_8 = SQUARE_MASKS[A8] | SQUARE_MASKS[B8] | SQUARE_MASKS[C8] | SQUARE_MASKS[D8] | SQUARE_MASKS[E8] | SQUARE_MASKS[F8] | SQUARE_MASKS[G8] | SQUARE_MASKS[H8];

    NOT_A_FILE = ~A_FILE;
    NOT_H_FILE = ~H_FILE;
    NOT_RANK_1 = ~RANK_1;
    NOT_RANK_8 = ~RANK_8;

    B_FILE = SQUARE_MASKS[B1] | SQUARE_MASKS[B2] | SQUARE_MASKS[B3] | SQUARE_MASKS[B4] | SQUARE_MASKS[B5] | SQUARE_MASKS[B6] | SQUARE_MASKS[B7] | SQUARE_MASKS[B8];
    G_FILE = SQUARE_MASKS[G1] | SQUARE_MASKS[G2] | SQUARE_MASKS[G3] | SQUARE_MASKS[G4] | SQUARE_MASKS[G5] | SQUARE_MASKS[G6] | SQUARE_MASKS[G7] | SQUARE_MASKS[G8];
    RANK_2 = SQUARE_MASKS[A2] | SQUARE_MASKS[B2] | SQUARE_MASKS[C2] | SQUARE_MASKS[D2] | SQUARE_MASKS[E2] | SQUARE_MASKS[F2] | SQUARE_MASKS[G2] | SQUARE_MASKS[H2];
    RANK_7 = SQUARE_MASKS[A7] | SQUARE_MASKS[B7] | SQUARE_MASKS[C7] | SQUARE_MASKS[D7] | SQUARE_MASKS[E7] | SQUARE_MASKS[F7] | SQUARE_MASKS[G7] | SQUARE_MASKS[H7];

    NOT_B_FILE = ~B_FILE;
    NOT_G_FILE = ~G_FILE;
    NOT_RANK_2 = ~RANK_2;
    NOT_RANK_7 = ~RANK_7;


    // initialize king moves.  If the square is not on the edge of the board, then the squares, +7, +8, +9,
    // -1, +1, -7, -8, and -9 are the directions a king can move.
    for (i=0; i<64; i++) {
        KING_MOVES[i] = 0; //initialize;
        cursquare = SQUARE_MASKS[i];

        if (cursquare & NOT_A_FILE) {
            KING_MOVES[i] |= SQUARE_MASKS[i-1];
        }
        if (cursquare & NOT_H_FILE) {
            KING_MOVES[i] |= SQUARE_MASKS[i+1];
        }
        if (cursquare & NOT_RANK_1) {
            KING_MOVES[i] |= SQUARE_MASKS[i-8];
        }
        if (cursquare & NOT_RANK_8) {
            KING_MOVES[i] |= SQUARE_MASKS[i+8];
        }
        if ((cursquare & NOT_A_FILE) && (cursquare & NOT_RANK_1)) {
            KING_MOVES[i] |= SQUARE_MASKS[i-9];
        }
        if ((cursquare & NOT_A_FILE) && (cursquare & NOT_RANK_8)) {
            KING_MOVES[i] |= SQUARE_MASKS[i-7];
        }
        if ((cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_1)) {
            KING_MOVES[i] |= SQUARE_MASKS[i+7];
        }
        if ((cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_8)) {
            KING_MOVES[i] |= SQUARE_MASKS[i+9];
        }
    }

    // initalize knight moves.  Knight moves are -10, +6, +15, +17, +10, -6, -15, -17
    for (i=0; i<64; i++) {
        KNIGHT_MOVES[i] = 0;
        cursquare = SQUARE_MASKS[i];

        if ((cursquare & NOT_RANK_1) && (cursquare & NOT_A_FILE) && (cursquare & NOT_B_FILE)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS[i-10];
        }
        if ((cursquare & NOT_RANK_8) && (cursquare & NOT_A_FILE) && (cursquare & NOT_B_FILE)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS[i+6];
        }
        if ((cursquare & NOT_RANK_1) && (cursquare & NOT_RANK_2) && (cursquare & NOT_A_FILE)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS [i-17];
        }
        if ((cursquare & NOT_RANK_1) && (cursquare & NOT_RANK_2) && (cursquare & NOT_H_FILE)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS [i-15];
        }
        if ((cursquare & NOT_G_FILE) && (cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_1)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS [i-6];
        }
        if ((cursquare & NOT_G_FILE) && (cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_8)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS [i+10];
        }
        if ((cursquare & NOT_RANK_7) && (cursquare & NOT_RANK_8) && (cursquare & NOT_A_FILE)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS [i+15];
        }
        if ((cursquare & NOT_RANK_7) && (cursquare & NOT_RANK_8) && (cursquare & NOT_H_FILE)) {
            KNIGHT_MOVES[i] |= SQUARE_MASKS [i+17];
        }
    }




    return true;
}


void const_bitmask_verify() {
    int i;

    for (i=0; i<64; i++) {
        //printf("Mask[%d] = %lx\nNot Mask[%d] = %lx\n\n",i, SQUARE_MASKS[i], i, NOT_MASKS[i]);
        printf("Knight moves %d = %lx\n", i, KNIGHT_MOVES[i]);
    }

    printf("A FILE = %lx\n", A_FILE);
    printf("H FILE = %lx\n", H_FILE);
    printf("RANK 1 = %lx\n", RANK_1);
    printf("RANK 8 = %lx\n", RANK_8);

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
    memset(pbb->piece_boards, 0, sizeof(pbb->piece_boards));
    pbb->piece_boards[EMPTY_SQUARES] = ~(pbb->piece_boards[ALL_PIECES]);
    pbb->ep_target = 0;
    pbb->halfmove_clock = 0;
    pbb->fullmove_number = 1;
    pbb->attrs = 0;
    pbb->halfmoves_completed = 0;
    pbb->hash = 0;
    pbb->wk_pos = -1; // something in there to mean there is no piece of this type on the board.
    pbb->bk_pos = -1;
}

int algebraic_to_bitpos(const char alg[2])
{
    return (8 * ((alg[1] - '0') - 1)) + (alg[0] - 'a');
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
                pbb->wk_pos = cur_square;
                got_wk = true;
            } else if (piece == BK) {
                pbb->bk_pos = cur_square;
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
    pbb->piece_boards[EMPTY_SQUARES] = ~(pbb->piece_boards[ALL_PIECES]);

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

int generate_bb_move_list(const struct bitChessBoard *pbb, MoveList *ml)
{
    MOVELIST_CLEAR(ml);
    uint_64 openmoves;
    uint_64 capturemoves;
    int kingpos;
    int dest;
    int good_color;  // the "good" team is the team moving.
    int bad_color;
    int piece_captured, curpos;
    uint_64 piece_list;
    // TODO - see if keeping these 12 ints & 12 masks around is faster than recomputing each time we access the pbb->piece_boards array.
    // TODO - an alternative would be an "if white" with mirrored "if black" code with the constants hardcoded in each one.
    int good_p, good_n, good_b, good_r, good_q, good_k;
    int bad_p, bad_n, bad_b, bad_r, bad_q, bad_k;
    uint_64 good_pmask, good_nmask, good_bmask, good_rmask, good_qmask, good_kmask;
    uint_64 bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask;
    uint_64 good_team_mask, bad_team_mask;
    bool move_is_check;
    

    if (pbb->attrs & W_TO_MOVE) {
        good_color = WHITE;
        bad_color = BLACK;
        kingpos = pbb->wk_pos;
        good_p = WP; good_n = WN; good_b = WB; good_r = WR; good_q = WQ; good_k = WK;
        bad_p = BP; bad_n = BN; bad_b = BB; bad_r = BR; bad_q = BQ; bad_k = BK;
    } else {
        good_color = BLACK;
        bad_color = WHITE;
        kingpos = pbb->bk_pos;
        good_p = BP; good_n = BN; good_b = BB; good_r = BR; good_q = BQ; good_k = BK;
        bad_p = WP; bad_n = WN; bad_b = WB; bad_r = WR; bad_q = WQ; bad_k = WK;
    }

    good_pmask = pbb->piece_boards[good_p];
    good_nmask = pbb->piece_boards[good_n];
    good_bmask = pbb->piece_boards[good_b];
    good_rmask = pbb->piece_boards[good_r];
    good_qmask = pbb->piece_boards[good_q];
    good_kmask = pbb->piece_boards[good_k];
    bad_pmask = pbb->piece_boards[bad_p];
    bad_nmask = pbb->piece_boards[bad_n];
    bad_bmask = pbb->piece_boards[bad_b];
    bad_rmask = pbb->piece_boards[bad_r];
    bad_qmask = pbb->piece_boards[bad_q];
    bad_kmask = pbb->piece_boards[bad_k];
    good_team_mask = pbb->piece_boards[good_color];
    bad_team_mask = pbb->piece_boards[bad_color];

    // generate standard king moves
    openmoves = KING_MOVES[kingpos] & pbb->piece_boards[EMPTY_SQUARES];
    while (openmoves) {
        dest = pop_lsb(&openmoves);
        // if move would put king adjacent to enemy king, it is not a valid move
        if (!(KING_MOVES[dest] & bad_kmask)) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, good_k, 0, 0, 0));
        }
    }
    // generate king captures
    capturemoves = KING_MOVES[kingpos] & bad_team_mask;
    while (capturemoves) {
        dest = pop_lsb(&capturemoves);
        if (bad_pmask & SQUARE_MASKS[dest]) {
            piece_captured = bad_p;
        } else if (bad_nmask & SQUARE_MASKS[dest]) {
            piece_captured = bad_n;
        } else if (bad_bmask & SQUARE_MASKS[dest]) {
            piece_captured = bad_b;
        } else if (bad_rmask & SQUARE_MASKS[dest]) {
            piece_captured = bad_r;
        } else if (bad_qmask & SQUARE_MASKS[dest]) {
            piece_captured = bad_q;
        } else {
            assert(false); // can't capture the king legally ever.
        }
        MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, good_k, piece_captured, 0, 0));
    }

    // generate knight moves and captures
    piece_list = good_nmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        openmoves = KNIGHT_MOVES[curpos] & pbb->piece_boards[EMPTY_SQUARES];
        while(openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_n, 0, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = KNIGHT_MOVES[curpos] & bad_team_mask;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            if (bad_pmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_p;
            } else if (bad_nmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_n;
            } else if (bad_bmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_b;
            } else if (bad_rmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_r;
            } else if (bad_qmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_q;
            } else {
                assert(false); // can't capture the king legally ever.
            }
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_n, piece_captured, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate pawn moves and captures.  Pawns are only pieces where different colors have different moves.
    // TODO - try to streamline it so that we can do it in one loop instead of two mirrored loops
    piece_list = good_pmask;
    if (good_p == WP) {
        // get single push moves first
        openmoves = (piece_list << 8) & pbb->piece_boards[EMPTY_SQUARES];
        while(openmoves) {
            dest = pop_lsb(&openmoves);
            if (dest >= 56) {
                //promotions
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, WQ, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, WN, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, WR, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, WB, 0));
            }
            else {
                move_is_check = (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest+7] & bad_kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest+9] & bad_kmask)));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, 0, move_is_check ? MOVE_CHECK : 0));
            }
        }
        // get double push moves
        openmoves = ((piece_list & RANK_2) << 16) & pbb->piece_boards[EMPTY_SQUARES];
        while(openmoves) {
            dest = pop_lsb(&openmoves);
            move_is_check = (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest+7] & bad_kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest+9] & bad_kmask)));
            MOVELIST_ADD(ml, CREATE_MOVE(dest-16, dest, WP, 0, 0, move_is_check ? MOVE_DOUBLE_PAWN | MOVE_CHECK : MOVE_DOUBLE_PAWN));
        }
        // get left captures
        capturemoves = ((piece_list & NOT_A_FILE) << 7) & bad_team_mask;
        while(capturemoves) {
            dest = pop_lsb(&capturemoves);
            if (bad_pmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_p;
            } else if (bad_nmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_n;
            } else if (bad_bmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_b;
            } else if (bad_rmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_r;
            } else if (bad_qmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_q;
            } else {
                assert(false); // can't capture the king legally ever.
            }
            if (dest >= 56) {
                //promotions
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, WQ, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, WN, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, WR, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, WB, 0));
            }
            else {
                move_is_check = (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest+7] & bad_kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest+9] & bad_kmask)));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, 0, move_is_check ? MOVE_CHECK : 0));
            }
        }
        // get right captures
        capturemoves = ((piece_list & NOT_H_FILE) << 9) & bad_team_mask;
        while(capturemoves) {
            dest = pop_lsb(&capturemoves);
            if (bad_pmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_p;
            } else if (bad_nmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_n;
            } else if (bad_bmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_b;
            } else if (bad_rmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_r;
            } else if (bad_qmask & SQUARE_MASKS[dest]) {
                piece_captured = bad_q;
            } else {
                assert(false); // can't capture the king legally ever.
            }
            if (dest >= 56) {
                //promotions
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, WQ, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, WN, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, WR, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, WB, 0));
            }
            else {
                move_is_check = (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest+7] & bad_kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest+9] & bad_kmask)));
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, 0, move_is_check ? MOVE_CHECK : 0));
            }
        }

        // en-passant captures
        capturemoves = ((piece_list & NOT_A_FILE) << 7) & SQUARE_MASKS[pbb->ep_target];
        while(capturemoves) {
            dest = pop_lsb(&capturemoves); // should only be one of these, but repeating the same pattern for consistency
            assert (SQUARE_MASKS[dest-8] & bad_pmask);
            move_is_check = (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest+7] & bad_kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest+9] & bad_kmask)));
            MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, BP, 0, move_is_check ? MOVE_EN_PASSANT | MOVE_CHECK : MOVE_EN_PASSANT));
        }
        capturemoves = ((piece_list & NOT_H_FILE) << 9) & SQUARE_MASKS[pbb->ep_target];
        while(capturemoves) {
            dest = pop_lsb(&capturemoves); // should only be one of these, but repeating the same pattern for consistency
            assert (SQUARE_MASKS[dest-8] & bad_pmask);
            move_is_check = (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest + 7] & bad_kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest + 9] & bad_kmask)));
            MOVELIST_ADD(ml, CREATE_MOVE(dest - 9, dest, WP, BP, 0, move_is_check ? MOVE_EN_PASSANT | MOVE_CHECK : MOVE_EN_PASSANT));
        }
    } else {

    }

}