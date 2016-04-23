#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "hash.h"
#include "magicmoves.h"

const uint_64 SQUARE_MASKS[64] = {
        0x1ull, 0x2ull, 0x4ull, 0x8ull,
        0x10ull, 0x20ull, 0x40ull, 0x80ull,
        0x100ull, 0x200ull, 0x400ull, 0x800ull,
        0x1000ull, 0x2000ull, 0x4000ull, 0x8000ull,
        0x10000ull, 0x20000ull, 0x40000ull, 0x80000ull,
        0x100000ull, 0x200000ull, 0x400000ull, 0x800000ull,
        0x1000000ull, 0x2000000ull, 0x4000000ull, 0x8000000ull,
        0x10000000ull, 0x20000000ull, 0x40000000ull, 0x80000000ull,
        0x100000000ull, 0x200000000ull, 0x400000000ull, 0x800000000ull,
        0x1000000000ull, 0x2000000000ull, 0x4000000000ull, 0x8000000000ull,
        0x10000000000ull, 0x20000000000ull, 0x40000000000ull, 0x80000000000ull,
        0x100000000000ull, 0x200000000000ull, 0x400000000000ull, 0x800000000000ull,
        0x1000000000000ull, 0x2000000000000ull, 0x4000000000000ull, 0x8000000000000ull,
        0x10000000000000ull, 0x20000000000000ull, 0x40000000000000ull, 0x80000000000000ull,
        0x100000000000000ull, 0x200000000000000ull, 0x400000000000000ull, 0x800000000000000ull,
        0x1000000000000000ull, 0x2000000000000000ull, 0x4000000000000000ull, 0x8000000000000000ull
};


const uint_64 NOT_MASKS[64] = {
        0xfffffffffffffffeull, 0xfffffffffffffffdull, 0xfffffffffffffffbull, 0xfffffffffffffff7ull,
        0xffffffffffffffefull, 0xffffffffffffffdfull, 0xffffffffffffffbfull, 0xffffffffffffff7full,
        0xfffffffffffffeffull, 0xfffffffffffffdffull, 0xfffffffffffffbffull, 0xfffffffffffff7ffull,
        0xffffffffffffefffull, 0xffffffffffffdfffull, 0xffffffffffffbfffull, 0xffffffffffff7fffull,
        0xfffffffffffeffffull, 0xfffffffffffdffffull, 0xfffffffffffbffffull, 0xfffffffffff7ffffull,
        0xffffffffffefffffull, 0xffffffffffdfffffull, 0xffffffffffbfffffull, 0xffffffffff7fffffull,
        0xfffffffffeffffffull, 0xfffffffffdffffffull, 0xfffffffffbffffffull, 0xfffffffff7ffffffull,
        0xffffffffefffffffull, 0xffffffffdfffffffull, 0xffffffffbfffffffull, 0xffffffff7fffffffull,
        0xfffffffeffffffffull, 0xfffffffdffffffffull, 0xfffffffbffffffffull, 0xfffffff7ffffffffull,
        0xffffffefffffffffull, 0xffffffdfffffffffull, 0xffffffbfffffffffull, 0xffffff7fffffffffull,
        0xfffffeffffffffffull, 0xfffffdffffffffffull, 0xfffffbffffffffffull, 0xfffff7ffffffffffull,
        0xffffefffffffffffull, 0xffffdfffffffffffull, 0xffffbfffffffffffull, 0xffff7fffffffffffull,
        0xfffeffffffffffffull, 0xfffdffffffffffffull, 0xfffbffffffffffffull, 0xfff7ffffffffffffull,
        0xffefffffffffffffull, 0xffdfffffffffffffull, 0xffbfffffffffffffull, 0xff7fffffffffffffull,
        0xfeffffffffffffffull, 0xfdffffffffffffffull, 0xfbffffffffffffffull, 0xf7ffffffffffffffull,
        0xefffffffffffffffull, 0xdfffffffffffffffull, 0xbfffffffffffffffull, 0x7fffffffffffffffull
};


// Masks to find the edges of the board, or squares not on a given edge.
// Masks for 2 squares in from an edge, used to compute knight moves
const uint_64 A_FILE = 0x101010101010101ul;
const uint_64 B_FILE = 0x202020202020202ul;
const uint_64 G_FILE = 0x4040404040404040ul;
const uint_64 H_FILE = 0x8080808080808080ul;
const uint_64 RANK_1 = 0xfful;
const uint_64 RANK_2 = 0xff00ul;
const uint_64 RANK_7 = 0xff000000000000ul;
const uint_64 RANK_8 = 0xff00000000000000ul;
const uint_64 NOT_A_FILE = 0xfefefefefefefefeul;
const uint_64 NOT_B_FILE = 0xfdfdfdfdfdfdfdfdul;
const uint_64 NOT_G_FILE = 0xbfbfbfbfbfbfbfbful;
const uint_64 NOT_H_FILE = 0x7f7f7f7f7f7f7f7ful;
const uint_64 NOT_RANK_1 = 0xffffffffffffff00ul;
const uint_64 NOT_RANK_2 = 0xffffffffffff00fful;
const uint_64 NOT_RANK_7 = 0xff00fffffffffffful;
const uint_64 NOT_RANK_8 = 0xfffffffffffffful;
const uint_64 ON_AN_EDGE = 0xff818181818181fful;
const uint_64 NOT_ANY_EDGE = 0x7e7e7e7e7e7e00ul;


// Masks for use in move generation
const uint_64 KNIGHT_MOVES[64] = {
        0x20400ull, 0x50800ull, 0xa1100ull, 0x142200ull,
        0x284400ull, 0x508800ull, 0xa01000ull, 0x402000ull,
        0x2040004ull, 0x5080008ull, 0xa110011ull, 0x14220022ull,
        0x28440044ull, 0x50880088ull, 0xa0100010ull, 0x40200020ull,
        0x204000402ull, 0x508000805ull, 0xa1100110aull, 0x1422002214ull,
        0x2844004428ull, 0x5088008850ull, 0xa0100010a0ull, 0x4020002040ull,
        0x20400040200ull, 0x50800080500ull, 0xa1100110a00ull, 0x142200221400ull,
        0x284400442800ull, 0x508800885000ull, 0xa0100010a000ull, 0x402000204000ull,
        0x2040004020000ull, 0x5080008050000ull, 0xa1100110a0000ull, 0x14220022140000ull,
        0x28440044280000ull, 0x50880088500000ull, 0xa0100010a00000ull, 0x40200020400000ull,
        0x204000402000000ull, 0x508000805000000ull, 0xa1100110a000000ull, 0x1422002214000000ull,
        0x2844004428000000ull, 0x5088008850000000ull, 0xa0100010a0000000ull, 0x4020002040000000ull,
        0x400040200000000ull, 0x800080500000000ull, 0x1100110a00000000ull, 0x2200221400000000ull,
        0x4400442800000000ull, 0x8800885000000000ull, 0x100010a000000000ull, 0x2000204000000000ull,
        0x4020000000000ull, 0x8050000000000ull, 0x110a0000000000ull, 0x22140000000000ull,
        0x44280000000000ull, 0x88500000000000ull, 0x10a00000000000ull, 0x20400000000000ull
};


const uint_64 KING_MOVES[64] = {
        0x302ull, 0x705ull, 0xe0aull, 0x1c14ull,
        0x3828ull, 0x7050ull, 0xe0a0ull, 0xc040ull,
        0x30203ull, 0x70507ull, 0xe0a0eull, 0x1c141cull,
        0x382838ull, 0x705070ull, 0xe0a0e0ull, 0xc040c0ull,
        0x3020300ull, 0x7050700ull, 0xe0a0e00ull, 0x1c141c00ull,
        0x38283800ull, 0x70507000ull, 0xe0a0e000ull, 0xc040c000ull,
        0x302030000ull, 0x705070000ull, 0xe0a0e0000ull, 0x1c141c0000ull,
        0x3828380000ull, 0x7050700000ull, 0xe0a0e00000ull, 0xc040c00000ull,
        0x30203000000ull, 0x70507000000ull, 0xe0a0e000000ull, 0x1c141c000000ull,
        0x382838000000ull, 0x705070000000ull, 0xe0a0e0000000ull, 0xc040c0000000ull,
        0x3020300000000ull, 0x7050700000000ull, 0xe0a0e00000000ull, 0x1c141c00000000ull,
        0x38283800000000ull, 0x70507000000000ull, 0xe0a0e000000000ull, 0xc040c000000000ull,
        0x302030000000000ull, 0x705070000000000ull, 0xe0a0e0000000000ull, 0x1c141c0000000000ull,
        0x3828380000000000ull, 0x7050700000000000ull, 0xe0a0e00000000000ull, 0xc040c00000000000ull,
        0x203000000000000ull, 0x507000000000000ull, 0xa0e000000000000ull, 0x141c000000000000ull,
        0x2838000000000000ull, 0x5070000000000000ull, 0xa0e0000000000000ull, 0x40c0000000000000ull
};

const uint_64 SLIDER_MOVES[64] = {
        0x1010101010101feull, 0x2020202020202fdull, 0x4040404040404fbull, 0x8080808080808f7ull,
        0x10101010101010efull, 0x20202020202020dfull, 0x40404040404040bfull, 0x808080808080807full,
        0x10101010101fe01ull, 0x20202020202fd02ull, 0x40404040404fb04ull, 0x80808080808f708ull,
        0x101010101010ef10ull, 0x202020202020df20ull, 0x404040404040bf40ull, 0x8080808080807f80ull,
        0x101010101fe0101ull, 0x202020202fd0202ull, 0x404040404fb0404ull, 0x808080808f70808ull,
        0x1010101010ef1010ull, 0x2020202020df2020ull, 0x4040404040bf4040ull, 0x80808080807f8080ull,
        0x1010101fe010101ull, 0x2020202fd020202ull, 0x4040404fb040404ull, 0x8080808f7080808ull,
        0x10101010ef101010ull, 0x20202020df202020ull, 0x40404040bf404040ull, 0x808080807f808080ull,
        0x10101fe01010101ull, 0x20202fd02020202ull, 0x40404fb04040404ull, 0x80808f708080808ull,
        0x101010ef10101010ull, 0x202020df20202020ull, 0x404040bf40404040ull, 0x8080807f80808080ull,
        0x101fe0101010101ull, 0x202fd0202020202ull, 0x404fb0404040404ull, 0x808f70808080808ull,
        0x1010ef1010101010ull, 0x2020df2020202020ull, 0x4040bf4040404040ull, 0x80807f8080808080ull,
        0x1fe010101010101ull, 0x2fd020202020202ull, 0x4fb040404040404ull, 0x8f7080808080808ull,
        0x10ef101010101010ull, 0x20df202020202020ull, 0x40bf404040404040ull, 0x807f808080808080ull,
        0xfe01010101010101ull, 0xfd02020202020202ull, 0xfb04040404040404ull, 0xf708080808080808ull,
        0xef10101010101010ull, 0xdf20202020202020ull, 0xbf40404040404040ull, 0x7f80808080808080ull
};


const uint_64 DIAGONAL_MOVES[64] = {
        0x8040201008040200ull, 0x80402010080500ull, 0x804020110a00ull, 0x8041221400ull,
        0x182442800ull, 0x10204885000ull, 0x102040810a000ull, 0x102040810204000ull,
        0x4020100804020002ull, 0x8040201008050005ull, 0x804020110a000aull, 0x804122140014ull,
        0x18244280028ull, 0x1020488500050ull, 0x102040810a000a0ull, 0x204081020400040ull,
        0x2010080402000204ull, 0x4020100805000508ull, 0x804020110a000a11ull, 0x80412214001422ull,
        0x1824428002844ull, 0x102048850005088ull, 0x2040810a000a010ull, 0x408102040004020ull,
        0x1008040200020408ull, 0x2010080500050810ull, 0x4020110a000a1120ull, 0x8041221400142241ull,
        0x182442800284482ull, 0x204885000508804ull, 0x40810a000a01008ull, 0x810204000402010ull,
        0x804020002040810ull, 0x1008050005081020ull, 0x20110a000a112040ull, 0x4122140014224180ull,
        0x8244280028448201ull, 0x488500050880402ull, 0x810a000a0100804ull, 0x1020400040201008ull,
        0x402000204081020ull, 0x805000508102040ull, 0x110a000a11204080ull, 0x2214001422418000ull,
        0x4428002844820100ull, 0x8850005088040201ull, 0x10a000a010080402ull, 0x2040004020100804ull,
        0x200020408102040ull, 0x500050810204080ull, 0xa000a1120408000ull, 0x1400142241800000ull,
        0x2800284482010000ull, 0x5000508804020100ull, 0xa000a01008040201ull, 0x4000402010080402ull,
        0x2040810204080ull, 0x5081020408000ull, 0xa112040800000ull, 0x14224180000000ull,
        0x28448201000000ull, 0x50880402010000ull, 0xa0100804020100ull, 0x40201008040201ull
};

const uint_64 WHITE_PAWN_ATTACKSTO[64] = {
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x200ull, 0x500ull, 0xa00ull, 0x1400ull,
        0x2800ull, 0x5000ull, 0xa000ull, 0x4000ull,
        0x20000ull, 0x50000ull, 0xa0000ull, 0x140000ull,
        0x280000ull, 0x500000ull, 0xa00000ull, 0x400000ull,
        0x2000000ull, 0x5000000ull, 0xa000000ull, 0x14000000ull,
        0x28000000ull, 0x50000000ull, 0xa0000000ull, 0x40000000ull,
        0x200000000ull, 0x500000000ull, 0xa00000000ull, 0x1400000000ull,
        0x2800000000ull, 0x5000000000ull, 0xa000000000ull, 0x4000000000ull,
        0x20000000000ull, 0x50000000000ull, 0xa0000000000ull, 0x140000000000ull,
        0x280000000000ull, 0x500000000000ull, 0xa00000000000ull, 0x400000000000ull,
        0x2000000000000ull, 0x5000000000000ull, 0xa000000000000ull, 0x14000000000000ull,
        0x28000000000000ull, 0x50000000000000ull, 0xa0000000000000ull, 0x40000000000000ull
};
const uint_64 BLACK_PAWN_ATTACKSTO[64] = {
        0x200ull, 0x500ull, 0xa00ull, 0x1400ull,
        0x2800ull, 0x5000ull, 0xa000ull, 0x4000ull,
        0x20000ull, 0x50000ull, 0xa0000ull, 0x140000ull,
        0x280000ull, 0x500000ull, 0xa00000ull, 0x400000ull,
        0x2000000ull, 0x5000000ull, 0xa000000ull, 0x14000000ull,
        0x28000000ull, 0x50000000ull, 0xa0000000ull, 0x40000000ull,
        0x200000000ull, 0x500000000ull, 0xa00000000ull, 0x1400000000ull,
        0x2800000000ull, 0x5000000000ull, 0xa000000000ull, 0x4000000000ull,
        0x20000000000ull, 0x50000000000ull, 0xa0000000000ull, 0x140000000000ull,
        0x280000000000ull, 0x500000000000ull, 0xa00000000000ull, 0x400000000000ull,
        0x2000000000000ull, 0x5000000000000ull, 0xa000000000000ull, 0x14000000000000ull,
        0x28000000000000ull, 0x50000000000000ull, 0xa0000000000000ull, 0x40000000000000ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull,
        0x0ull, 0x0ull, 0x0ull, 0x0ull
};


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
    int i,j;
    uint_64 cursquare;

    initmagicmoves();
    // All code moved to bitboard_constant_generation.c

    return true;
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

static inline int which_piece_on_square(const int color, int dest, const uint_64 pmask, const uint_64 nmask, const uint_64 bmask, const uint_64 rmask, const uint_64 qmask, const uint_64 kmask)
{
    // since color white = 0 and black pawn = white pawn + black, can use trick of returning WP + color, for example, to get pawn color returned.

    if (pmask & SQUARE_MASKS[dest]) {
        return (WP + color);
    } else if (nmask & SQUARE_MASKS[dest]) {
        return (WN + color);
    } else if (bmask & SQUARE_MASKS[dest]) {
        return (WB + color);
    } else if (rmask & SQUARE_MASKS[dest]) {
        return (WR + color);
    } else if (qmask & SQUARE_MASKS[dest]) {
        return (WQ + color);
    } else if (kmask & SQUARE_MASKS[dest]) {
        return (WK + color);
    } else {
        assert(false); // square is empty
    }
}

static inline bool pawncheck (const int color, const int dest, const uint_64 kmask)
{
    if (color == WHITE) {
        return (((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest+7] & kmask)) || ((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest+9] & kmask)));
    } else {
        return (((SQUARE_MASKS[dest] & NOT_H_FILE) && (SQUARE_MASKS[dest-7] & kmask)) || ((SQUARE_MASKS[dest] & NOT_A_FILE) && (SQUARE_MASKS[dest-9] & kmask)));
    }
}

static inline uint_64 pieces_attacking_square(const struct bitChessBoard *pbb, int square, int color_attacking)
{
    uint_64 pawn_attacks, knight_attacks, diag_attacks, slide_attacks, king_attacks;

    // TODO - rewrite so it has no branching
    if (color_attacking == WHITE) {
        pawn_attacks = WHITE_PAWN_ATTACKSTO[square] & pbb->piece_boards[WP];
    } else {
        pawn_attacks = BLACK_PAWN_ATTACKSTO[square] & pbb->piece_boards[BP];
    }
    knight_attacks = KNIGHT_MOVES[square] & pbb->piece_boards[WN+color_attacking];
    king_attacks = KING_MOVES[square] & pbb->piece_boards[WK+color_attacking];
    diag_attacks = Bmagic(square, pbb->piece_boards[ALL_PIECES]) & (pbb->piece_boards[WB+color_attacking] | pbb->piece_boards[WQ+color_attacking]);
    slide_attacks = Rmagic(square, pbb->piece_boards[ALL_PIECES]) & (pbb->piece_boards[WR+color_attacking] | pbb->piece_boards[WQ+color_attacking]);

    return (pawn_attacks | knight_attacks | diag_attacks | slide_attacks | king_attacks);
}

static inline bool side_is_in_check(const struct bitChessBoard *pbb, int color_attacking)
{
    if (color_attacking == WHITE) {
        return pieces_attacking_square(pbb, pbb->bk_pos, WHITE);
    } else {
        return pieces_attacking_square(pbb, pbb->wk_pos, BLACK);
    }
}



int generate_bb_move_list(const struct bitChessBoard *pbb, MoveList *ml)
{
    uint_64 openmoves;
    uint_64 capturemoves;
    uint_64 allmoves;
    uint_64 moves;
    uint_64 pawnmovemasks[6];
    int pawnstartfactor[6];
    int pawnmoveflags[6];
    uint_64 promorank;
    int kingpos;
    int start, dest, i, end, flags, piece_moving;
    int good_color;  // the "good" team is the team moving.
    int bad_color;
    int piece_captured, curpos;
    uint_64 piece_list;
    // TODO - see if keeping these 12 ints & 12 masks around is faster than recomputing each time we access the pbb->piece_boards array.
    // TODO - an alternative would be an "if white" with mirrored "if black" code with the constants hardcoded in each one.
    int good_p, good_n, good_b, good_r, good_q, good_k;
    int bad_p, bad_n, bad_b, bad_r, bad_q, bad_k;
    uint_64 good_pmask, good_nmask, good_bmask, good_rmask, good_qmask;
    uint_64 bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask;
    uint_64 good_team_mask, bad_team_mask;
    bool move_is_check, removed_move;
    bool can_castle_q, can_castle_k;
    struct bitChessBoard tmp;
    Move m;
    int p7;


    MOVELIST_CLEAR(ml);

    if (pbb->attrs & W_TO_MOVE) {
        good_color = WHITE;
        bad_color = BLACK;
        kingpos = pbb->wk_pos;
        good_p = WP; good_n = WN; good_b = WB; good_r = WR; good_q = WQ; good_k = WK;
        bad_p = BP; bad_n = BN; bad_b = BB; bad_r = BR; bad_q = BQ; bad_k = BK;
        can_castle_q = pbb->attrs & W_CASTLE_QUEEN;
        can_castle_k = pbb->attrs & W_CASTLE_KING;

    } else {
        good_color = BLACK;
        bad_color = WHITE;
        kingpos = pbb->bk_pos;
        good_p = BP; good_n = BN; good_b = BB; good_r = BR; good_q = BQ; good_k = BK;
        bad_p = WP; bad_n = WN; bad_b = WB; bad_r = WR; bad_q = WQ; bad_k = WK;
        can_castle_q = pbb->attrs & B_CASTLE_QUEEN;
        can_castle_k = pbb->attrs & W_CASTLE_KING;
    }

    good_pmask = pbb->piece_boards[good_p];
    good_nmask = pbb->piece_boards[good_n];
    good_bmask = pbb->piece_boards[good_b];
    good_rmask = pbb->piece_boards[good_r];
    good_qmask = pbb->piece_boards[good_q];
    bad_pmask = pbb->piece_boards[bad_p];
    bad_nmask = pbb->piece_boards[bad_n];
    bad_bmask = pbb->piece_boards[bad_b];
    bad_rmask = pbb->piece_boards[bad_r];
    bad_qmask = pbb->piece_boards[bad_q];
    bad_kmask = pbb->piece_boards[bad_k];
    bad_team_mask = pbb->piece_boards[bad_color];

    // generate bishop moves
    piece_list = good_bmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Bmagic(curpos, pbb->piece_boards[ALL_PIECES]);
        openmoves = pbb->piece_boards[EMPTY_SQUARES] & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_b, 0, 0, (Bmagic(dest, pbb->piece_boards[ALL_PIECES]) & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_b, 0, 0, 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_b, piece_captured, 0, (Bmagic(dest, pbb->piece_boards[ALL_PIECES]) & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_b, piece_captured, 0, 0));
        }
    }

    // generate rook moves
    piece_list = good_rmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Rmagic(curpos, pbb->piece_boards[ALL_PIECES]);
        openmoves = pbb->piece_boards[EMPTY_SQUARES] & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_r, 0, 0, (Rmagic(dest, pbb->piece_boards[ALL_PIECES]) & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_r, 0, 0, 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_r, piece_captured, 0, (Rmagic(dest, pbb->piece_boards[ALL_PIECES]) & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_r, piece_captured, 0, 0));
        }
    }

    // generate queen moves
    piece_list = good_qmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Rmagic(curpos, pbb->piece_boards[ALL_PIECES]) | Bmagic(curpos, pbb->piece_boards[ALL_PIECES]);
        openmoves = pbb->piece_boards[EMPTY_SQUARES] & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_q, 0, 0, ((Rmagic(dest, pbb->piece_boards[ALL_PIECES]) | (Bmagic(curpos, pbb->piece_boards[ALL_PIECES]))) & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_q, 0, 0, 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_r, piece_captured, 0, ((Rmagic(dest, pbb->piece_boards[ALL_PIECES]) | (Bmagic(curpos, pbb->piece_boards[ALL_PIECES]))) & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_r, piece_captured, 0, 0));
        }
    }

    // generate standard king moves
    openmoves = KING_MOVES[kingpos] & pbb->piece_boards[EMPTY_SQUARES];
    while (openmoves) {
        dest = pop_lsb(&openmoves);
        // if move would put king adjacent to enemy king, it is not a valid move
        if (!(KING_MOVES[dest] & bad_kmask)) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, good_k, 0, 0, 0));
        }
    }

    // generate castles
    if (can_castle_k) {
        // king must be in its home square so no need to test it.  if the rook is in its home file it must be in its home square as well.
        // it may be missing since we do not flip "can castle" false properly on rook captures from their home squares, as it's slow for a rare case.
        if ((good_rmask & H_FILE) &&
            !(pbb->piece_boards[ALL_PIECES] & (SQUARE_MASKS[kingpos + 1] | SQUARE_MASKS[kingpos + 2]))) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, kingpos + 2, good_k, 0, 0, MOVE_CASTLE));
        }
    }
    if (can_castle_q) {
        if ((good_rmask & A_FILE) && !(pbb->piece_boards[ALL_PIECES] & (SQUARE_MASKS[kingpos-1] | SQUARE_MASKS[kingpos-2] | SQUARE_MASKS[kingpos-3]))) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, kingpos-2, good_k, 0, 0, MOVE_CASTLE));
        }
    }


    // generate king captures
    capturemoves = KING_MOVES[kingpos] & bad_team_mask;
    while (capturemoves) {
        dest = pop_lsb(&capturemoves);
        piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
        // if move would put king adjacent to enemy king, it is not a valid move
        if (!(KING_MOVES[dest] & bad_kmask)) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, good_k, piece_captured, 0, 0));
        }
    }

    // generate knight moves and captures
    piece_list = good_nmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        openmoves = KNIGHT_MOVES[curpos] & pbb->piece_boards[EMPTY_SQUARES];
        while(openmoves) {
            dest = pop_lsb(&openmoves);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_n, 0, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_n, 0, 0, 0));
        }
        capturemoves = KNIGHT_MOVES[curpos] & bad_team_mask;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            // MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_n, piece_captured, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, good_n, piece_captured, 0, 0));
        }
    }

    // generate pawn moves and captures.  Pawns are only pieces where different colors have different moves.

    piece_list = good_pmask;
    if (good_color == WHITE) {
        promorank = RANK_8;
        // array of 6 pawn moves.  Single push, double push, capture left, capture right, capture left en passant, capture right en passant.
        pawnmovemasks[0] = (piece_list << 8) & pbb->piece_boards[EMPTY_SQUARES];
        pawnstartfactor[0] = -8;
        pawnmoveflags[0] = 0;
        pawnmovemasks[1] = ((piece_list & RANK_2) << 16) & pbb->piece_boards[EMPTY_SQUARES];
        pawnstartfactor[1] = -16;
        pawnmoveflags[1] = MOVE_DOUBLE_PAWN;
        pawnmovemasks[2] = ((piece_list & NOT_A_FILE) << 7) & bad_team_mask;
        pawnstartfactor[2] = -7;
        pawnmoveflags[2] = 0;
        pawnmovemasks[3] = ((piece_list & NOT_H_FILE) << 9) & bad_team_mask;
        pawnstartfactor[3] = -9;
        pawnmoveflags[3] = 0;
        if (pbb->ep_target) {
            pawnmovemasks[4] = ((piece_list & NOT_A_FILE) << 7) & SQUARE_MASKS[pbb->ep_target];
            pawnstartfactor[4] = -7;
            pawnmoveflags[4] = MOVE_EN_PASSANT;
            pawnmovemasks[5] = ((piece_list & NOT_H_FILE) << 9) & SQUARE_MASKS[pbb->ep_target];
            pawnstartfactor[5] = -9;
            pawnmoveflags[5] = MOVE_EN_PASSANT;
        } else {
            pawnmovemasks[4] = 0;
            pawnmovemasks[5] = 0;
        }
    } else {
        promorank = RANK_1;
        pawnmovemasks[0] = (piece_list >> 8) & pbb->piece_boards[EMPTY_SQUARES];
        pawnstartfactor[0] = 8;
        pawnmoveflags[0] = 0;
        pawnmovemasks[1] = ((piece_list & RANK_7) >> 16) & pbb->piece_boards[EMPTY_SQUARES];
        pawnstartfactor[1] = 16;
        pawnmoveflags[1] = MOVE_DOUBLE_PAWN;
        pawnmovemasks[2] = ((piece_list & NOT_H_FILE) >> 7) & bad_team_mask;
        pawnstartfactor[2] = 7;
        pawnmoveflags[2] = 0;
        pawnmovemasks[3] = ((piece_list & NOT_A_FILE) >> 9) & bad_team_mask;
        pawnstartfactor[3] = 9;
        pawnmoveflags[3] = 0;
        if (pbb->ep_target) {
            pawnmovemasks[4] = ((piece_list & NOT_H_FILE) >> 7) & SQUARE_MASKS[pbb->ep_target];
            pawnstartfactor[4] = 7;
            pawnmoveflags[4] = MOVE_EN_PASSANT;
            pawnmovemasks[5] = ((piece_list & NOT_A_FILE) >> 9) & SQUARE_MASKS[pbb->ep_target];
            pawnstartfactor[5] = 9;
            pawnmoveflags[5] = MOVE_EN_PASSANT;
        } else {
            pawnmovemasks[4] = 0;
            pawnmovemasks[5] = 0;
        }

    }

    for (i = 0; i < 6; i ++) {
        moves = pawnmovemasks[i];
        while(moves) {
            dest = pop_lsb(&moves);
            start = dest + pawnstartfactor[i];
            if (i <= 1) {
                piece_captured = 0;
            } else if (i <= 3) {
                piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            } else {
                piece_captured = bad_p;
            }
            if (SQUARE_MASKS[dest] & promorank) {
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, good_p, piece_captured, good_q, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, good_p, piece_captured, good_n, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, good_p, piece_captured, good_r, 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, good_p, piece_captured, good_b, 0));
            }
            else {
                // MOVELIST_ADD(ml, CREATE_MOVE(start, dest, good_p, piece_captured, 0, pawncheck(good_color, dest, bad_kmask) ? (pawnmoveflags[i] | MOVE_CHECK) : pawnmoveflags[i]));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, good_p, piece_captured, 0, pawnmoveflags[i]));
            }
        }
    }

    // now we will remove any illegal moves, and mark any moves that put the enemy in check.
    for (i=ml->size-1; i>=0; i--) {
        tmp = *pbb;
        m = ml->moves[i];
        start = GET_START(m);
        end = GET_END(m);
        flags = GET_FLAGS(m);
        piece_moving = GET_PIECE_MOVING(m);
        p7 = PIECE_BITS(piece_moving);
        apply_bb_move(&tmp, m);

        removed_move = false;
        if (side_is_in_check(pbb, bad_color)) {
            movelist_remove(ml, i);
            removed_move = true;
        } else if (flags & MOVE_CASTLE) {
            if (pieces_attacking_square(pbb, (start + end) / 2, bad_color)) {
                movelist_remove(ml, i);
                removed_move = true;
            }
        }

        if (!removed_move) {
            if (side_is_in_check(pbb, good_color)) {
                ml->moves[i] = m | ((Move)(MOVE_CHECK) << MOVE_FLAGS_SHIFT);
            }
        }
    }
}


bool apply_bb_move(struct bitChessBoard *pbb, Move m)
{

    int start, end, piece_moving, piece_captured, promoted_to, move_flags;

    start = GET_START(m);
    end = GET_END(m);
    piece_captured = GET_PIECE_CAPTURED(m);
    promoted_to = GET_PROMOTED_TO(m);
    piece_moving = GET_PIECE_MOVING(m);
    move_flags = GET_FLAGS(m);


    // Macros are GET_START, GET_END, GET_PIECE_MOVING, GET_PIECE_CAPTURED, GET_PROMOTED_TO, and GET_FLAGS
    int color_moving = piece_moving & BLACK;

    pbb->piece_boards[piece_moving] &= NOT_MASKS[start];
    if (promoted_to) {
        pbb->piece_boards[promoted_to] |= SQUARE_MASKS[end];
    } else {
        pbb->piece_boards[piece_moving] |= SQUARE_MASKS[end];
    }

    if (piece_moving == WK) {
        pbb->wk_pos = end;
    } else if (piece_moving == BK) {
        pbb->bk_pos = end;
    }

    if (piece_captured) {
        if (move_flags & MOVE_EN_PASSANT) {
            if (color_moving == WHITE) {
                pbb->piece_boards[BP] &= NOT_MASKS[end-10];
            } else {
                pbb->piece_boards[WP] &= NOT_MASKS[end+10];
            }
        } else {
            pbb->piece_boards[piece_captured] &= NOT_MASKS[end];
        }
    }

    if (move_flags & MOVE_DOUBLE_PAWN) {
        if (color_moving == WHITE) {
            pbb->ep_target = end - 10;
        } else {
            pbb->ep_target = end + 10;
        }
    } else {
        pbb->ep_target = 0;

        if (move_flags & MOVE_CASTLE) {
            if (color_moving) {
                pbb->attrs &= ~(B_CASTLE_KING | B_CASTLE_QUEEN);
            } else {
                pbb->attrs &= ~(W_CASTLE_KING | W_CASTLE_QUEEN);
            }
            if (end > start) {
                // king side
                pbb->piece_boards[WR + color_moving] &= NOT_MASKS[start+3];
                pbb->piece_boards[WR + color_moving] |= SQUARE_MASKS[start+1];
            } else {
                pbb->piece_boards[WR + color_moving] &= NOT_MASKS[start-4];
                pbb->piece_boards[WR + color_moving] |= SQUARE_MASKS[start-1];
            }
        } else {
            if (pbb->attrs & (W_CASTLE_KING | W_CASTLE_QUEEN)) {
                if (piece_moving == WK) {
                    pbb->attrs &= ~(W_CASTLE_KING | W_CASTLE_QUEEN);
                } else if (piece_moving == WR) {
                    if (start == A1) {
                        pbb->attrs &= (~W_CASTLE_QUEEN);
                    } else if (start == H1) {
                        pbb->attrs &= (~W_CASTLE_KING);
                    }
                }
            }
            if (pbb->attrs & (B_CASTLE_KING | B_CASTLE_QUEEN)) {
                if (piece_moving == BK) {
                    pbb->attrs &= ~(B_CASTLE_KING | B_CASTLE_QUEEN);
                } else if (piece_moving == BR) {
                    if (start == A8) {
                        pbb->attrs &= (~B_CASTLE_QUEEN);
                    } else if (start == H8) {
                        pbb->attrs &= (~B_CASTLE_KING);
                    }
                }
            }
        }
    }

    if (move_flags & MOVE_CHECK) {
        pbb -> attrs |= BOARD_IN_CHECK;
    } else {
        pbb -> attrs &= (~BOARD_IN_CHECK);
    }

    if (piece_moving == WP || piece_moving == BP || piece_captured) {
        pbb -> halfmove_clock = 0;
    } else {
        pbb -> halfmove_clock ++;
    }

    if (!(pbb->attrs & W_TO_MOVE)) {
        pbb -> fullmove_number ++;
        pbb->attrs |= W_TO_MOVE;
    } else {
        pbb->attrs &= (~W_TO_MOVE);
    }

    pbb->move_history[(pbb->halfmoves_completed)++] = m;
    if (pbb->halfmoves_completed >= MAX_MOVE_HISTORY) {
        //TODO - something better here?  At least this won't coredump.  But we lose history after 128 moves.
        pbb->halfmoves_completed = 0;
    }

    // TODO - optimize this as we can sharply reduce the number of operations
    if (color_moving == WHITE || GET_PIECE_CAPTURED(m)) {
        pbb->piece_boards[WHITE] = pbb->piece_boards[WP] | pbb->piece_boards[WN] | pbb->piece_boards[WB] | pbb->piece_boards[WR] | pbb->piece_boards[WQ] | pbb->piece_boards[WK];
    }
    if (color_moving == BLACK || GET_PIECE_CAPTURED(m)) {
        pbb->piece_boards[BLACK] = pbb->piece_boards[BP] | pbb->piece_boards[BN] | pbb->piece_boards[BB] | pbb->piece_boards[BR] | pbb->piece_boards[BQ] | pbb->piece_boards[BK];
    }
    pbb->piece_boards[ALL_PIECES] = pbb->piece_boards[WHITE] | pbb->piece_boards[BLACK];
    pbb->piece_boards[EMPTY_SQUARES] = ~(pbb->piece_boards[ALL_PIECES]);

}