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
const uint_64 RANK_3 = 0xff0000ul;
const uint_64 RANK_6 = 0xff0000000000ul;
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

uint_64 SQUARES_BETWEEN[64][64];  // too big to justify making a constant;s

const int castle_move_mask[64] =
    {~W_CASTLE_QUEEN, ~0, ~0, ~0, ~(W_CASTLE_QUEEN|W_CASTLE_KING), ~0, ~0, ~W_CASTLE_KING,
    ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
    ~0, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
    ~B_CASTLE_QUEEN, ~0, ~0, ~0, ~(B_CASTLE_QUEEN|B_CASTLE_KING), ~0, ~0, ~B_CASTLE_KING
};

uint_64 castle_empty_square_mask[9][2];
uint_64 castle_safe_square_mask[9][2];


// in the move generation routine, I need to reference "good" and "bad" pieces.  I had a branch in there to set these
// variables based on the side moving.  This allows me to eliminate the branch.
typedef enum piece_goodness {
    good_p, good_n, good_b, good_r, good_q, good_k, bad_p, bad_n, bad_b, bad_r, bad_q, bad_k
} piece_goodness;
// in this array we only use the 0 and 8 rows for white & black.
const int piece_relations [9][12] = {
        {WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0,0,0,0,0},
        {BP, BN, BB, BR, BQ, BK, WP, WN, WB, WR, WQ, WK}
};






bool const_bitmask_init()
{
    int i,j, c;
    uint_64 cursquare;

    initmagicmoves();
    // Most code moved to bitboard_constant_generation.c

    castle_empty_square_mask[WHITE][0] = SQUARE_MASKS[F1] | SQUARE_MASKS[G1];
    castle_empty_square_mask[WHITE][1] = SQUARE_MASKS[B1] | SQUARE_MASKS[C1] | SQUARE_MASKS[D1];
    castle_empty_square_mask[BLACK][0] = SQUARE_MASKS[F8] | SQUARE_MASKS[G8];
    castle_empty_square_mask[BLACK][1] = SQUARE_MASKS[B8] | SQUARE_MASKS[C8] | SQUARE_MASKS[D8];

    castle_safe_square_mask[WHITE][0] = SQUARE_MASKS[E1] | SQUARE_MASKS[F1] | SQUARE_MASKS[G1];
    castle_safe_square_mask[WHITE][1] = SQUARE_MASKS[E1] | SQUARE_MASKS[D1] | SQUARE_MASKS[C1];
    castle_safe_square_mask[BLACK][0] = SQUARE_MASKS[E8] | SQUARE_MASKS[F8] | SQUARE_MASKS[G8];
    castle_safe_square_mask[BLACK][1] = SQUARE_MASKS[E8] | SQUARE_MASKS[D8] | SQUARE_MASKS[C8];

    for (i=0; i<64; i++){
        for (j=0;j<64;j ++) {
            SQUARES_BETWEEN[i][j] = 0; // initialize
        }
    }

    for (i=0; i<64; i++) {

        if (i < 48) {
            j = i + 16;
            while (j < 64) {
                for (c = i + 8; c < j; c += 8) {
                    SQUARES_BETWEEN[i][j] |= SQUARE_MASKS[c];
                    SQUARES_BETWEEN[j][i] |= SQUARE_MASKS[c];
                }
                j+=8;
            }
        }
        if ((i % 8) <= 5) {
            j = i + 2;
            while ((j < 64) && (j%8 != 0)) {
                for (c=i+1; c < j; c++) {
                    SQUARES_BETWEEN[i][j] |= SQUARE_MASKS[c];
                    SQUARES_BETWEEN[j][i] |= SQUARE_MASKS[c];
                }
                j++;
            }
        }
        if (!(SQUARE_MASKS[i] & (G_FILE | H_FILE))) {
            j = i + 18;
            while((j<64) && (j%8 !=0)) {
                for (c=i+9; c<j; c+=9) {
                    SQUARES_BETWEEN[j][i] |= SQUARE_MASKS[c];
                    SQUARES_BETWEEN[i][j] |= SQUARE_MASKS[c];
                }
                j+=9;
            }
        }
        if (!(SQUARE_MASKS[i] & (A_FILE | B_FILE))) {
            j = i + 14;
            while((j<64) && (j%8 != 7)) {
                for (c=i+7; c<j; c+=7) {
                    SQUARES_BETWEEN[i][j] |= SQUARE_MASKS[c];
                    SQUARES_BETWEEN[j][i] |= SQUARE_MASKS[c];
                }
                j+=7;
            }

        }
    }

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
    pbb->castling = 0;
    pbb->in_check = false;
    pbb->side_to_move = WHITE;
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

static inline bool side_is_in_check(const struct bitChessBoard *pbb, int color_defending)
{
    if (color_defending == WHITE) {
        return pieces_attacking_square(pbb, pbb->wk_pos, BLACK);
    } else {
        return pieces_attacking_square(pbb, pbb->bk_pos, WHITE);
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
        pbb->side_to_move = WHITE;
    } else if (cur_char == 'b'){
        pbb->side_to_move = BLACK;
    } else {
        return false; // invalid - either white or black must be on move
    }

    cur_char = fen[++curpos];
    while (cur_char != '\0' && cur_char == ' ') {
        cur_char = fen[++curpos];
    }

    while (cur_char != '\0' && cur_char != ' ') {
        switch (cur_char) {
            case ('K'):
                pbb->castling = pbb->castling | W_CASTLE_KING;
                break;
            case ('Q'):
                pbb->castling = pbb->castling | W_CASTLE_QUEEN;
                break;
            case ('k'):
                pbb->castling = pbb->castling | B_CASTLE_KING;
                break;
            case ('q'):
                pbb->castling = pbb->castling | B_CASTLE_QUEEN;
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

    pbb->piece_boards[WHITE] = pbb->piece_boards[WP] | pbb->piece_boards[WN] | pbb->piece_boards[WB] | pbb->piece_boards[WR] | pbb->piece_boards[WQ] | pbb->piece_boards[WK];
    pbb->piece_boards[BLACK] = pbb->piece_boards[BP] | pbb->piece_boards[BN] | pbb->piece_boards[BB] | pbb->piece_boards[BR] | pbb->piece_boards[BQ] | pbb->piece_boards[BK];
    pbb->piece_boards[ALL_PIECES] = pbb->piece_boards[WHITE] | pbb->piece_boards[BLACK];
    pbb->piece_boards[EMPTY_SQUARES] = ~(pbb->piece_boards[ALL_PIECES]);

    if (side_is_in_check(pbb, pbb->side_to_move)) {
        pbb->in_check = true;
    }

    pbb->hash = compute_bitboard_hash(pbb);


#ifdef VALIDATE_BITBOARD_EACH_STEP
    assert(validate_board_sanity(pbb));
#endif

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
    if (pbb->side_to_move == WHITE) {
        ret[curchar++] = 'w';
    } else {
        ret[curchar++] = 'b';
    }
    ret[curchar++] = ' ';
    if (pbb->castling & W_CASTLE_KING) {
        ret[curchar++] = 'K';
        hascastle = true;
    }
    if (pbb->castling & W_CASTLE_QUEEN) {
        ret[curchar++] = 'Q';
        hascastle = true;
    }
    if (pbb->castling & B_CASTLE_KING) {
        ret[curchar++] = 'k';
        hascastle = true;
    }
    if (pbb->castling & B_CASTLE_QUEEN) {
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

uint_64 generate_bb_pinned_list(const struct bitChessBoard *pbb, int square, int color_of_blockers, int color_of_attackers)
{
    uint_64 ret, unpinned_attacks, pinning_attackers;

    ret = 0;
    // diags first
    unpinned_attacks = Bmagic(square, pbb->piece_boards[ALL_PIECES]);
    pinning_attackers = Bmagic(square, pbb->piece_boards[ALL_PIECES] & (~unpinned_attacks)) & (pbb->piece_boards[WQ+color_of_attackers] | pbb->piece_boards[WB+color_of_attackers]);
    while(pinning_attackers) {
        ret |= (SQUARES_BETWEEN[square][pop_lsb(&pinning_attackers)] & pbb->piece_boards[color_of_blockers]);
    }
    // repeat for sliders
    unpinned_attacks = Rmagic(square, pbb->piece_boards[ALL_PIECES]);
    pinning_attackers = Rmagic(square, pbb->piece_boards[ALL_PIECES] & (~unpinned_attacks)) & (pbb->piece_boards[WQ+color_of_attackers] | pbb->piece_boards[WR+color_of_attackers]);
    while(pinning_attackers) {
        ret |= (SQUARES_BETWEEN[square][pop_lsb(&pinning_attackers)] & pbb->piece_boards[color_of_blockers]);
    }

    return ret;
}

void generate_bb_ep_moves(const struct bitChessBoard *pbb, struct MoveList *ml)
{
    uint_64 capturemoves, start;
    struct bitChessBoard tmpBoard;
    Move tmpMove;

    if (pbb->side_to_move == WHITE) {
        capturemoves = pbb->piece_boards[WP] & WHITE_PAWN_ATTACKSTO[pbb->ep_target];
        while (capturemoves) {
            tmpMove = CREATE_MOVE(pop_lsb(&capturemoves), pbb->ep_target, WP, BP, 0, (SQUARE_MASKS[pbb->ep_target] & WHITE_PAWN_ATTACKSTO[pbb->bk_pos]) ? MOVE_CHECK | MOVE_EN_PASSANT : MOVE_EN_PASSANT);
            tmpBoard = *pbb;
            apply_bb_move(&tmpBoard, tmpMove);
            // TODO try to simplify this calculation
            if (!side_is_in_check(&tmpBoard, WHITE))
                MOVELIST_ADD(ml, tmpMove);
        }
    } else {
        capturemoves = pbb->piece_boards[BP] & BLACK_PAWN_ATTACKSTO[pbb->ep_target];
        while (capturemoves) {
            tmpMove = CREATE_MOVE(pop_lsb(&capturemoves), pbb->ep_target, BP, WP, 0, (SQUARE_MASKS[pbb->ep_target] & BLACK_PAWN_ATTACKSTO[pbb->wk_pos]) ? MOVE_CHECK | MOVE_EN_PASSANT : MOVE_EN_PASSANT);
            tmpBoard = *pbb;
            apply_bb_move(&tmpBoard, tmpMove);
            if (!side_is_in_check(&tmpBoard, BLACK)) {
                MOVELIST_ADD(ml, tmpMove);
            }
        }
    }
}


void generate_bb_move_list_in_check(const struct bitChessBoard *pbb, struct MoveList *ml)
{
    uint_64 openmoves;
    uint_64 capturemoves;
    uint_64 allmoves;
    uint_64 single_pushmoves, double_pushmoves, capture_leftmoves, capture_rightmoves;
    int kingpos, bad_kpos;
    int start, dest, i, piece_moving;
    int good_color;  // the "good" team is the team moving.
    int bad_color;
    int piece_captured, curpos;
    uint_64 piece_list;
    // TODO - see if keeping these 12 ints & 12 masks around is faster than recomputing each time we access the pbb->piece_boards array.
    // TODO - an alternative would be an "if white" with mirrored "if black" code with the constants hardcoded in each one.
    uint_64 good_rmask;
    uint_64 bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask;
    uint_64 bad_team_mask;
    uint_64 pinned_piece_mask, discovered_check_mask;
    bool removed_move;
    struct bitChessBoard tmp;
    Move m;
    uint_64 tmpRmask, tmpBmask, emptyMask, allMask, attackedMask, t;

    MOVELIST_CLEAR(ml);

    good_color = pbb->side_to_move;
    bad_color = good_color ^ BLACK;



    good_rmask = pbb->piece_boards[piece_relations[good_color][good_r]];
    bad_pmask = pbb->piece_boards[piece_relations[good_color][bad_p]];
    bad_nmask = pbb->piece_boards[piece_relations[good_color][bad_n]];
    bad_bmask = pbb->piece_boards[piece_relations[good_color][bad_b]];
    bad_rmask = pbb->piece_boards[piece_relations[good_color][bad_r]];
    bad_qmask = pbb->piece_boards[piece_relations[good_color][bad_q]];
    bad_kmask = pbb->piece_boards[piece_relations[good_color][bad_k]];
    bad_team_mask = pbb->piece_boards[bad_color];

    emptyMask = pbb->piece_boards[EMPTY_SQUARES];
    allMask = pbb->piece_boards[ALL_PIECES];

    // get all squares attacked by the enemy
    attackedMask = 0;
    t = bad_nmask;
    while(t) {
        attackedMask |= KNIGHT_MOVES[pop_lsb(&t)];
    }
    t = bad_kmask;
    while(t) {
        attackedMask |= KING_MOVES[pop_lsb(&t)];
    }
    t = bad_bmask;
    while(t) {
        attackedMask |= Bmagic(pop_lsb(&t), allMask);
    }
    t = bad_rmask;
    while(t) {
        attackedMask |= Rmagic(pop_lsb(&t), allMask);
    }
    t = bad_qmask;
    while(t) {
        i = pop_lsb(&t);
        attackedMask |= Bmagic(i, allMask);
        attackedMask |= Rmagic(i, allMask);
    }
    // Pawns will be added as soon as we get into the if statement below since it varies by color and I want to do one branch




    // pawn moves and castling are moves that vary based on color, other moves are constant.  To limit branching we do all the color-specific moves first
    if (pbb->side_to_move == WHITE) {
        attackedMask |= ((pbb->piece_boards[BP] & NOT_H_FILE) >> 7);
        attackedMask |= ((pbb->piece_boards[BP] & NOT_A_FILE) >> 9);

        kingpos = pbb->wk_pos;
        bad_kpos = pbb->bk_pos;
        piece_list = pbb->piece_boards[WP];
        single_pushmoves = (piece_list << 8) & emptyMask;
        double_pushmoves = ((single_pushmoves & RANK_3) << 8) & emptyMask;
        capture_leftmoves = ((piece_list & NOT_A_FILE) << 7) & bad_team_mask;
        capture_rightmoves = ((piece_list & NOT_H_FILE) << 9) & bad_team_mask;

        while(single_pushmoves) {
            dest = pop_lsb(&single_pushmoves);
            if (SQUARE_MASKS[dest] & RANK_8) {
                start = dest - 8;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while(double_pushmoves) {
            dest = pop_lsb(&double_pushmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(dest-16, dest, WP, 0, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_DOUBLE_PAWN | MOVE_CHECK : MOVE_DOUBLE_PAWN));
        }
        while(capture_leftmoves) {
            dest = pop_lsb(&capture_leftmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_8) {
                start = dest - 7;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while(capture_rightmoves) {
            dest = pop_lsb(&capture_rightmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_8) {
                start = dest - 9;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }



    } else {
        attackedMask |= ((pbb->piece_boards[WP] & NOT_A_FILE) << 7);
        attackedMask |= ((pbb->piece_boards[WP] & NOT_H_FILE) << 9);

        kingpos = pbb->bk_pos;
        bad_kpos = pbb->wk_pos;
        piece_list = pbb->piece_boards[BP];
        single_pushmoves = (piece_list >> 8) & emptyMask;
        double_pushmoves = ((single_pushmoves & RANK_6) >> 8) & emptyMask;
        capture_leftmoves = ((piece_list & NOT_H_FILE) >> 7) & bad_team_mask;
        capture_rightmoves = ((piece_list & NOT_A_FILE) >> 9) & bad_team_mask;

        while(single_pushmoves) {
            dest = pop_lsb(&single_pushmoves);
            if (SQUARE_MASKS[dest] & RANK_1) {
                start = dest + 8;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest+8, dest, BP, 0, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while(double_pushmoves) {
            dest = pop_lsb(&double_pushmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(dest+16, dest, BP, 0, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_DOUBLE_PAWN | MOVE_CHECK : MOVE_DOUBLE_PAWN));
        }
        while (capture_leftmoves) {
            dest = pop_lsb(&capture_leftmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_1) {
                start = dest + 7;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest+7, dest, BP, piece_captured, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while (capture_rightmoves) {
            dest = pop_lsb(&capture_rightmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_1) {
                start = dest + 9;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest+9, dest, BP, piece_captured, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
    }

    // these require the good/bad kpos to be set to be calculated.  If we need them before the above branch, then we need to move setting good/bad kpos so
    // it doesn't require a branch.
    pinned_piece_mask = generate_bb_pinned_list(pbb, kingpos, good_color, bad_color);
    discovered_check_mask = generate_bb_pinned_list(pbb, bad_kpos, good_color, good_color);


    // ep moves - super rare so no big hit by doing one potential extra branch to test color to move inside the pbb->eptarget.
    if_unlikely(pbb->ep_target) {
        generate_bb_ep_moves(pbb, ml);
    }



    // generate bishop moves
    piece_list = pbb->piece_boards[piece_relations[good_color][good_b]];
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Bmagic(curpos, pbb->piece_boards[ALL_PIECES]);
        openmoves = emptyMask & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_b], 0, 0, (Bmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_b], piece_captured, 0, (Bmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate rook moves
    piece_list = good_rmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Rmagic(curpos, allMask);
        openmoves = emptyMask & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_r], 0, 0, (Rmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_r], piece_captured, 0, (Rmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate queen moves
    piece_list = pbb->piece_boards[piece_relations[good_color][good_q]];
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Rmagic(curpos, allMask) | Bmagic(curpos, allMask);
        openmoves = emptyMask & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_q], 0, 0, ((Rmagic(dest, allMask) | (Bmagic(dest, allMask))) & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_q], piece_captured, 0, ((Rmagic(dest, allMask) | (Bmagic(dest, allMask))) & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate standard king moves
    openmoves = KING_MOVES[kingpos] & emptyMask;
    while (openmoves) {
        dest = pop_lsb(&openmoves);
        if (!(attackedMask & SQUARE_MASKS[dest])) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, piece_relations[good_color][good_k], 0, 0, 0));
        }
    }

    // generate king captures
    capturemoves = KING_MOVES[kingpos] & bad_team_mask;
    while (capturemoves) {
        dest = pop_lsb(&capturemoves);
        piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);

        if (!(attackedMask & SQUARE_MASKS[dest])) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, piece_relations[good_color][good_k], piece_captured, 0, 0));
        }
    }

    // generate knight moves and captures
    piece_list = pbb->piece_boards[piece_relations[good_color][good_n]];
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        openmoves = KNIGHT_MOVES[curpos] & emptyMask;
        while(openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_n], 0, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = KNIGHT_MOVES[curpos] & bad_team_mask;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_n], piece_captured, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
        }
    }





    // now we will remove any illegal moves, and mark any moves that put the enemy in check.  Unless you are already in check, the only positions
    // where you could move into check are king moves, moves of pinned pieces, or en-passant captures (because you could remove two pieces blocking king from check).
    // TODO - handle "currently in check" in a totally separate block, or limit the move gen to only generate legal "in-check" moves so we don't
    // generate a ton of moves that we just have to delete.
    for (i=ml->size-1; i>=0; i--) {
        tmp = *pbb;
        m = ml->moves[i];
        start = GET_START(m);
        piece_moving = GET_PIECE_MOVING(m);

        removed_move = false;
        apply_bb_move(&tmp, m);

        if (side_is_in_check(&tmp, good_color)) {
            movelist_remove(ml, i);
            removed_move = true;
        }

        if (!removed_move) {
            // if the move is legal, and it is one of the few cases where we didn't compute check when we made the move, compute it now.
            if ((SQUARE_MASKS[start] & discovered_check_mask) || (GET_FLAGS(m) & MOVE_EN_PASSANT)) {
                if (side_is_in_check(&tmp, bad_color)) {
                    ml->moves[i] |= ((Move) (MOVE_CHECK) << MOVE_FLAGS_SHIFT);
                }
            }
        }
    }
}


void generate_bb_move_list_normal(const struct bitChessBoard *pbb, struct MoveList *ml)
{
    uint_64 openmoves;
    uint_64 capturemoves;
    uint_64 allmoves;
    uint_64 single_pushmoves, double_pushmoves, capture_leftmoves, capture_rightmoves;
    int kingpos, bad_kpos;
    int start, dest, i, piece_moving;
    int good_color;  // the "good" team is the team moving.
    int bad_color;
    int piece_captured, curpos;
    uint_64 piece_list;
    // TODO - see if keeping these 12 ints & 12 masks around is faster than recomputing each time we access the pbb->piece_boards array.
    // TODO - an alternative would be an "if white" with mirrored "if black" code with the constants hardcoded in each one.
    uint_64 good_rmask;
    uint_64 bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask;
    uint_64 bad_team_mask;
    uint_64 pinned_piece_mask, discovered_check_mask;
    bool removed_move;
    bool can_castle_q, can_castle_k, applied_move;
    struct bitChessBoard tmp;
    Move m;
    uint_64 tmpRmask, tmpBmask, emptyMask, allMask, attackedMask, t;

    MOVELIST_CLEAR(ml);

    good_color = pbb->side_to_move;
    bad_color = good_color ^ BLACK;



    good_rmask = pbb->piece_boards[piece_relations[good_color][good_r]];
    bad_pmask = pbb->piece_boards[piece_relations[good_color][bad_p]];
    bad_nmask = pbb->piece_boards[piece_relations[good_color][bad_n]];
    bad_bmask = pbb->piece_boards[piece_relations[good_color][bad_b]];
    bad_rmask = pbb->piece_boards[piece_relations[good_color][bad_r]];
    bad_qmask = pbb->piece_boards[piece_relations[good_color][bad_q]];
    bad_kmask = pbb->piece_boards[piece_relations[good_color][bad_k]];
    bad_team_mask = pbb->piece_boards[bad_color];

    emptyMask = pbb->piece_boards[EMPTY_SQUARES];
    allMask = pbb->piece_boards[ALL_PIECES];

    // get all squares attacked by the enemy
    attackedMask = 0;
    t = bad_nmask;
    while(t) {
        attackedMask |= KNIGHT_MOVES[pop_lsb(&t)];
    }
    t = bad_kmask;
    while(t) {
        attackedMask |= KING_MOVES[pop_lsb(&t)];
    }
    t = bad_bmask;
    while(t) {
        attackedMask |= Bmagic(pop_lsb(&t), allMask);
    }
    t = bad_rmask;
    while(t) {
        attackedMask |= Rmagic(pop_lsb(&t), allMask);
    }
    t = bad_qmask;
    while(t) {
        i = pop_lsb(&t);
        attackedMask |= Bmagic(i, allMask);
        attackedMask |= Rmagic(i, allMask);
    }
    // Pawns will be added as soon as we get into the if statement below since it varies by color and I want to do one branch




    // pawn moves and castling are moves that vary based on color, other moves are constant.  To limit branching we do all the color-specific moves first
    if (pbb->side_to_move == WHITE) {
        attackedMask |= ((pbb->piece_boards[BP] & NOT_H_FILE) >> 7);
        attackedMask |= ((pbb->piece_boards[BP] & NOT_A_FILE) >> 9);

        kingpos = pbb->wk_pos;
        bad_kpos = pbb->bk_pos;
        can_castle_q = pbb->castling & W_CASTLE_QUEEN;
        can_castle_k = pbb->castling & W_CASTLE_KING;
        piece_list = pbb->piece_boards[WP];
        single_pushmoves = (piece_list << 8) & emptyMask;
        double_pushmoves = ((single_pushmoves & RANK_3) << 8) & emptyMask;
        capture_leftmoves = ((piece_list & NOT_A_FILE) << 7) & bad_team_mask;
        capture_rightmoves = ((piece_list & NOT_H_FILE) << 9) & bad_team_mask;

        while(single_pushmoves) {
            dest = pop_lsb(&single_pushmoves);
            if (SQUARE_MASKS[dest] & RANK_8) {
                start = dest - 8;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, 0, WB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest-8, dest, WP, 0, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while(double_pushmoves) {
            dest = pop_lsb(&double_pushmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(dest-16, dest, WP, 0, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_DOUBLE_PAWN | MOVE_CHECK : MOVE_DOUBLE_PAWN));
        }
        while(capture_leftmoves) {
            dest = pop_lsb(&capture_leftmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_8) {
                start = dest - 7;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest-7, dest, WP, piece_captured, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while(capture_rightmoves) {
            dest = pop_lsb(&capture_rightmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_8) {
                start = dest - 9;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, WP, piece_captured, WB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest-9, dest, WP, piece_captured, 0, SQUARE_MASKS[dest] & WHITE_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }



    } else {
        attackedMask |= ((pbb->piece_boards[WP] & NOT_A_FILE) << 7);
        attackedMask |= ((pbb->piece_boards[WP] & NOT_H_FILE) << 9);

        kingpos = pbb->bk_pos;
        bad_kpos = pbb->wk_pos;
        can_castle_q = pbb->castling & B_CASTLE_QUEEN;
        can_castle_k = pbb->castling & B_CASTLE_KING;
        piece_list = pbb->piece_boards[BP];
        single_pushmoves = (piece_list >> 8) & emptyMask;
        double_pushmoves = ((single_pushmoves & RANK_6) >> 8) & emptyMask;
        capture_leftmoves = ((piece_list & NOT_H_FILE) >> 7) & bad_team_mask;
        capture_rightmoves = ((piece_list & NOT_A_FILE) >> 9) & bad_team_mask;

        while(single_pushmoves) {
            dest = pop_lsb(&single_pushmoves);
            if (SQUARE_MASKS[dest] & RANK_1) {
                start = dest + 8;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, 0, BB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest+8, dest, BP, 0, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while(double_pushmoves) {
            dest = pop_lsb(&double_pushmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(dest+16, dest, BP, 0, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_DOUBLE_PAWN | MOVE_CHECK : MOVE_DOUBLE_PAWN));
        }
        while (capture_leftmoves) {
            dest = pop_lsb(&capture_leftmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_1) {
                start = dest + 7;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest+7, dest, BP, piece_captured, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
        while (capture_rightmoves) {
            dest = pop_lsb(&capture_rightmoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            if (SQUARE_MASKS[dest] & RANK_1) {
                start = dest + 9;
                tmpRmask = (Rmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                tmpBmask = (Bmagic(dest, allMask & NOT_MASKS[start])) & bad_kmask;
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BQ, (tmpRmask | tmpBmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BN, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BR, tmpRmask ? MOVE_CHECK : 0));
                MOVELIST_ADD(ml, CREATE_MOVE(start, dest, BP, piece_captured, BB, tmpBmask ? MOVE_CHECK : 0));
            } else {
                MOVELIST_ADD(ml, CREATE_MOVE(dest+9, dest, BP, piece_captured, 0, SQUARE_MASKS[dest] & BLACK_PAWN_ATTACKSTO[bad_kpos] ? MOVE_CHECK : 0));
            }
        }
    }

    // these require the good/bad kpos to be set to be calculated.  If we need them before the above branch, then we need to move setting good/bad kpos so
    // it doesn't require a branch.
    pinned_piece_mask = generate_bb_pinned_list(pbb, kingpos, good_color, bad_color);
    discovered_check_mask = generate_bb_pinned_list(pbb, bad_kpos, good_color, good_color);


    // ep moves - super rare so no big hit by doing one potential extra branch to test color to move inside the pbb->eptarget.
    if_unlikely(pbb->ep_target) {
        generate_bb_ep_moves(pbb, ml);
    }



    // generate bishop moves
    piece_list = pbb->piece_boards[piece_relations[good_color][good_b]];
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Bmagic(curpos, pbb->piece_boards[ALL_PIECES]);
        openmoves = emptyMask & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_b], 0, 0, (Bmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_b], piece_captured, 0, (Bmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate rook moves
    piece_list = good_rmask;
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Rmagic(curpos, allMask);
        openmoves = emptyMask & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_r], 0, 0, (Rmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_r], piece_captured, 0, (Rmagic(dest, allMask) & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate queen moves
    piece_list = pbb->piece_boards[piece_relations[good_color][good_q]];
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        allmoves = Rmagic(curpos, allMask) | Bmagic(curpos, allMask);
        openmoves = emptyMask & allmoves;
        while (openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_q], 0, 0, ((Rmagic(dest, allMask) | (Bmagic(dest, allMask))) & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = bad_team_mask & allmoves;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_q], piece_captured, 0, ((Rmagic(dest, allMask) | (Bmagic(dest, allMask))) & bad_kmask) ? MOVE_CHECK : 0));
        }
    }

    // generate standard king moves
    openmoves = KING_MOVES[kingpos] & emptyMask;
    while (openmoves) {
        dest = pop_lsb(&openmoves);
        if (!(attackedMask & SQUARE_MASKS[dest])) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, piece_relations[good_color][good_k], 0, 0, 0));
        }
    }

    // generate castles
    if (can_castle_k) {
        // king must be in its home square so no need to test it.  if the rook is in its home file it must be in its home square as well.
        // it may be missing since we do not flip "can castle" false properly on rook captures from their home squares, as it's slow for a rare case.
        if ((!(allMask & castle_empty_square_mask[good_color][0])) && (!(attackedMask & castle_safe_square_mask[good_color][0]))) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, kingpos + 2, piece_relations[good_color][good_k], 0, 0, (Rmagic(kingpos + 1, allMask) & bad_kmask) ? MOVE_CHECK | MOVE_CASTLE: MOVE_CASTLE));
        }
    }
    if (can_castle_q) {
        if ((!(allMask & castle_empty_square_mask[good_color][1])) && (!(attackedMask & castle_safe_square_mask[good_color][1]))) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, kingpos-2, piece_relations[good_color][good_k], 0, 0, (Rmagic(kingpos -1, allMask) & bad_kmask) ? MOVE_CHECK | MOVE_CASTLE: MOVE_CASTLE));
        }
    }


    // generate king captures
    capturemoves = KING_MOVES[kingpos] & bad_team_mask;
    while (capturemoves) {
        dest = pop_lsb(&capturemoves);
        piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);

        if (!(attackedMask & SQUARE_MASKS[dest])) {
            MOVELIST_ADD(ml, CREATE_MOVE(kingpos, dest, piece_relations[good_color][good_k], piece_captured, 0, 0));
        }
    }

    // generate knight moves and captures
    piece_list = pbb->piece_boards[piece_relations[good_color][good_n]];
    while(piece_list) {
        curpos = pop_lsb(&piece_list);
        openmoves = KNIGHT_MOVES[curpos] & emptyMask;
        while(openmoves) {
            dest = pop_lsb(&openmoves);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_n], 0, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
        }
        capturemoves = KNIGHT_MOVES[curpos] & bad_team_mask;
        while (capturemoves) {
            dest = pop_lsb(&capturemoves);
            piece_captured = which_piece_on_square(bad_color, dest, bad_pmask, bad_nmask, bad_bmask, bad_rmask, bad_qmask, bad_kmask);
            MOVELIST_ADD(ml, CREATE_MOVE(curpos, dest, piece_relations[good_color][good_n], piece_captured, 0, (KNIGHT_MOVES[dest] & bad_kmask) ? MOVE_CHECK : 0));
        }
    }





    // now we will remove any illegal moves, and mark any moves that put the enemy in check.  Unless you are already in check, the only positions
    // where you could move into check are king moves, moves of pinned pieces, or en-passant captures (because you could remove two pieces blocking king from check).
    // TODO - handle "currently in check" in a totally separate block, or limit the move gen to only generate legal "in-check" moves so we don't
    // generate a ton of moves that we just have to delete.
   for (i=ml->size-1; i>=0; i--) {
        tmp = *pbb;
        m = ml->moves[i];
        start = GET_START(m);
        piece_moving = GET_PIECE_MOVING(m);

        removed_move = false;
        applied_move = false;
        if (SQUARE_MASKS[start] & pinned_piece_mask) {
            apply_bb_move(&tmp, m);
            applied_move = true;

            if (side_is_in_check(&tmp, good_color)) {
                movelist_remove(ml, i);
                removed_move = true;
            }
        }
        if (!removed_move) {
            // if the move is legal, and it is one of the few cases where we didn't compute check when we made the move, compute it now.
            if ((SQUARE_MASKS[start] & discovered_check_mask) || (GET_FLAGS(m) & MOVE_EN_PASSANT)) {
                if (!applied_move) {
                    apply_bb_move(&tmp, m);
                }
                if (side_is_in_check(&tmp, bad_color)) {
                    ml->moves[i] |= ((Move) (MOVE_CHECK) << MOVE_FLAGS_SHIFT);
                }
            }
        }
   }
}

void generate_bb_move_list(const struct bitChessBoard *pbb, MoveList *ml)
{
    if (pbb->in_check) {
        generate_bb_move_list_in_check(pbb, ml);
    } else {
        generate_bb_move_list_normal(pbb, ml);
    }
}


bool apply_bb_move(struct bitChessBoard *pbb, Move m)
{

    int start, end, piece_moving, piece_captured, promoted_to, move_flags, ep_target;

    start = GET_START(m);
    end = GET_END(m);
    piece_captured = GET_PIECE_CAPTURED(m);
    promoted_to = GET_PROMOTED_TO(m);
    piece_moving = GET_PIECE_MOVING(m);
    move_flags = GET_FLAGS(m);

    // Macros are GET_START, GET_END, GET_PIECE_MOVING, GET_PIECE_CAPTURED, GET_PROMOTED_TO, and GET_FLAGS
    int color_moving = piece_moving & BLACK;

    pbb->piece_boards[piece_moving] &= NOT_MASKS[start];
#ifndef DISABLE_HASH
    pbb->hash ^= bb_piece_hash[piece_moving][start];
#endif

    if (promoted_to) {
        pbb->piece_boards[promoted_to] |= SQUARE_MASKS[end];
#ifndef DISABLE_HASH
        pbb->hash ^= bb_piece_hash[promoted_to][end];
#endif
    } else {
        pbb->piece_boards[piece_moving] |= SQUARE_MASKS[end];
#ifndef DISABLE_HASH
        pbb->hash ^= bb_piece_hash[piece_moving][end];
#endif
    }

    if (piece_moving == WK) {
        pbb->wk_pos = end;
    } else if (piece_moving == BK) {
        pbb->bk_pos = end;
    }

    if (piece_captured) {
        if_unlikely (move_flags & MOVE_EN_PASSANT) {
            if (color_moving == WHITE) {
                pbb->piece_boards[BP] &= NOT_MASKS[end-8];
#ifndef DISABLE_HASH
                pbb->hash ^= bb_piece_hash[BP][end-8];
#endif
            } else {
                pbb->piece_boards[WP] &= NOT_MASKS[end+8];
#ifndef DISABLE_HASH
                pbb->hash ^= bb_piece_hash[WP][end+8];
#endif
            }
        } else {
            pbb->piece_boards[piece_captured] &= NOT_MASKS[end];
#ifndef DISABLE_HASH
            pbb->hash ^= bb_piece_hash[piece_captured][end];
#endif
        }
    }

#ifndef DISABLE_HASH
    pbb->hash ^= bb_hash_enpassanttarget[pbb->ep_target];
#endif

    pbb->ep_target = 0;  // cheaper to set once then have "else" conditions on 2 branches inside

    if (move_flags & MOVE_DOUBLE_PAWN) {
        if (color_moving == WHITE) {
            // TODO - mask the attacksto pawn mask for the other side, and see if they can hit the ep_target, only setting the ep target
            // if it would be useful in the next ply.
            ep_target = end-8;
            if_unlikely(BLACK_PAWN_ATTACKSTO[ep_target] & pbb->piece_boards[BP]) {
                pbb->ep_target = ep_target;
#ifndef DISABLE_HASH
                pbb->hash ^= bb_hash_enpassanttarget[ep_target];
#endif
            }

        } else {
            if_unlikely(WHITE_PAWN_ATTACKSTO[end+8] & pbb->piece_boards[WP]) {
                ep_target = end+8;
                pbb->ep_target = ep_target;
#ifndef DISABLE_HASH
                pbb->hash ^= bb_hash_enpassanttarget[ep_target];
#endif
            }
        }
    } else {

        pbb->hash ^= bb_hash_castling[pbb->castling];
        pbb->castling &= castle_move_mask[start];
        pbb->castling &= castle_move_mask[end]; // this catches rook captures

        if (move_flags & MOVE_CASTLE) {

            if (end > start) {
                // king side
                pbb->piece_boards[WR + color_moving] &= NOT_MASKS[start+3];
#ifndef DISABLE_HASH
                pbb->hash ^= bb_piece_hash[WR+color_moving][start+3];
#endif
                pbb->piece_boards[WR + color_moving] |= SQUARE_MASKS[start+1];
#ifndef DISABLE_HASH
                pbb->hash ^= bb_piece_hash[WR+color_moving][start+1];
#endif
            } else {
                pbb->piece_boards[WR + color_moving] &= NOT_MASKS[start-4];
#ifndef DISABLE_HASH
                pbb->hash ^= bb_piece_hash[WR + color_moving][start-4];
#endif
                pbb->piece_boards[WR + color_moving] |= SQUARE_MASKS[start-1];
#ifndef DISABLE_HASH
                pbb->hash ^= bb_piece_hash[WR + color_moving][start-1];
#endif
            }
        }

#ifndef DISABLE_HASH
        pbb->hash ^= bb_hash_castling[pbb->castling];
#endif
    }

    pbb-> in_check = (move_flags & MOVE_CHECK);

    if (piece_moving == WP || piece_moving == BP || piece_captured) {
        pbb -> halfmove_clock = 0;
    } else {
        pbb -> halfmove_clock ++;
    }

    if (pbb->side_to_move == BLACK) {
        pbb -> fullmove_number ++;
        pbb->side_to_move = WHITE;
    } else {
        pbb->side_to_move = BLACK;
    }
#ifndef DISABLE_HASH
    pbb->hash ^= bb_hash_whitetomove;
#endif

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


#ifdef VALIDATE_BITBOARD_EACH_STEP
    assert(validate_board_sanity(pbb));
#ifndef DISABLE_HASH
    assert(pbb->hash == compute_bitboard_hash(pbb));
#endif
#endif

}

void debugprint_bb_move_history(const struct bitChessBoard *pbb) {
    int i;
    char *moveprint;

    for (i=0; i< pbb->halfmoves_completed; i++) {
        moveprint = pretty_print_bb_move(pbb->move_history[i]);
        if ((i+1) % 2 == 1) {
            printf("%d. %s", (i/2) + 1, moveprint);
        } else {
            printf(" %s\n", moveprint);
        }
        free(moveprint);
    }
    printf("\n\n");
}

bool validate_board_sanity(struct bitChessBoard *pbb)
{
    uint_64 errmask;
    bool ret = true;
    int pos;
    int i, j;
    char * tmpstr;

    errmask = pbb->piece_boards[WHITE] & pbb->piece_boards[BLACK];
    while (errmask) {
        pos = pop_lsb(&errmask);
        printf("white and black both occupy %d\n", pos);
        ret = false;
    }
    errmask = pbb->piece_boards[WHITE] & pbb->piece_boards[EMPTY_SQUARES];
    while (errmask) {
        pos = pop_lsb(&errmask);
        printf("white and empty both occupy %d\n", pos);
        ret = false;
    }
    errmask = pbb->piece_boards[BLACK] & pbb->piece_boards[EMPTY_SQUARES];
    while (errmask) {
        pos = pop_lsb(&errmask);
        printf("Empty and black both occupy %d\n", pos);
        ret = false;
    }
    errmask = pbb->piece_boards[EMPTY_SQUARES] & pbb->piece_boards[ALL_PIECES];
    while (errmask) {
        pos = pop_lsb(&errmask);
        printf("Empty and all both occupy %d\n", pos);
        ret = false;
    }
    // pieces are 1-6 and 9-14
    for (i=1; i<=13; i++) {
        if (i!=7 && i!=8) {
            for (j=i+1; j<= 14; j++) {
                if (j!=7 && j!= 8) {
                    errmask = pbb->piece_boards[i] & pbb->piece_boards[j];
                    while (errmask) {
                        pos = pop_lsb(&errmask);
                        printf("Pieces %d and %d both occupy %d\n", i, j, pos);
                        ret = false;
                    }

                }
            }
        }
    }

    errmask = pbb->piece_boards[WP] & (RANK_1 | RANK_8);
    while (errmask) {
        pos = pop_lsb(&errmask);
        printf("white pawn in illegal position %d\n", pos);
        ret = false;
    }
    errmask = pbb->piece_boards[BP] & (RANK_1 | RANK_8);
    while (errmask) {
        pos = pop_lsb(&errmask);
        printf("Black pawn in illegal position %d\n", pos);
        ret = false;
    }

    if (!ret) {
        tmpstr = convert_bitboard_to_fen(pbb);
        printf("Above errors were for board: %s\n", tmpstr);
        debugprint_bb_move_history(pbb);
    }

    return(ret);

}