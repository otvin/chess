/*
 * When I originally authored bitboard.h / bitboard.c I had arrays of unsigned longs that were computed.  However,
 * arrays of constant unsigned longs would perform faster.  So, the code that initialized the constants was moved here.
 * If you gcc this file, it will dump out test that you can then paste into bitboard.h to define the constants.
 */

#include <stdio.h>
// Bitboard-based definition

typedef unsigned long uint_64;

// The order of the enum shows how the bits in the bitboard map to squares.  Least significant bit would be
// square A1, most significant would be H8.
typedef enum boardlayout {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
} boardlayout;


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
uint_64 ON_AN_EDGE;
uint_64 NOT_ANY_EDGE;

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
uint_64 SLIDER_MOVES[64];
uint_64 DIAGONAL_MOVES[64];

// the squares that white pawns are on if they attack this space
uint_64 WHITE_PAWN_ATTACKSTO[64];
uint_64 BLACK_PAWN_ATTACKSTO[64];


void const_bitmask_init()
{
    int i,j;
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

    ON_AN_EDGE = A_FILE | H_FILE | RANK_1 | RANK_8;
    NOT_ANY_EDGE = ~ON_AN_EDGE;

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
            KING_MOVES[i] |= SQUARE_MASKS[i+7];
        }
        if ((cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_1)) {
            KING_MOVES[i] |= SQUARE_MASKS[i-7];
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

    // initialize slider moves.
    for (i=0; i<64; i++) {
        SLIDER_MOVES[i] =0;
        // left first:
        j=i;
        cursquare = SQUARE_MASKS[j];
        while(cursquare & NOT_A_FILE) {
            j -= 1;
            cursquare = SQUARE_MASKS[j];
            SLIDER_MOVES[i] |= cursquare;
        }
        j=i;
        cursquare = SQUARE_MASKS[j];
        while(cursquare & NOT_H_FILE) {
            j += 1;
            cursquare = SQUARE_MASKS[j];
            SLIDER_MOVES[i] |= cursquare;
        }
        j=i;
        cursquare = SQUARE_MASKS[j];
        while(cursquare & NOT_RANK_1) {
            j -= 8;
            cursquare = SQUARE_MASKS[j];
            SLIDER_MOVES[i] |= cursquare;
        }
        j=i;
        cursquare = SQUARE_MASKS[j];
        while(cursquare & NOT_RANK_8) {
            j += 8;
            cursquare = SQUARE_MASKS[j];
            SLIDER_MOVES[i] |= cursquare;
        }
    }

    // initialize diagonal moves.
    for (i=0; i<64; i++) {
        DIAGONAL_MOVES[i] = 0;

        // northeast first:
        j=i;
        cursquare = SQUARE_MASKS[j];
        while((cursquare & NOT_A_FILE) && (cursquare & NOT_RANK_8)) {
            j += 7;
            cursquare = SQUARE_MASKS[j];
            DIAGONAL_MOVES[i] |= cursquare;
        }
        j=i;
        cursquare = SQUARE_MASKS[j];
        while((cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_8)) {
            j += 9;
            cursquare = SQUARE_MASKS[j];
            DIAGONAL_MOVES[i] |= cursquare;
        }
        j=i;
        cursquare = SQUARE_MASKS[j];
        while((cursquare & NOT_A_FILE) && (cursquare & NOT_RANK_1)) {
            j -= 9;
            cursquare = SQUARE_MASKS[j];
            DIAGONAL_MOVES[i] |= cursquare;
        }
        j=i;
        cursquare = SQUARE_MASKS[j];
        while((cursquare & NOT_H_FILE) && (cursquare & NOT_RANK_1)) {
            j -= 7;
            cursquare = SQUARE_MASKS[j];
            DIAGONAL_MOVES[i] |= cursquare;
        }
    }

    for (i=0; i<64; i++) {
        WHITE_PAWN_ATTACKSTO[i] = 0;
        BLACK_PAWN_ATTACKSTO[i] = 0;

        if (i >= 16) {
            // white pawns can't attack anything in ranks 1 or 2;
            if (SQUARE_MASKS[i] & NOT_A_FILE) {
                WHITE_PAWN_ATTACKSTO[i] |= SQUARE_MASKS[i-9];
            }
            if (SQUARE_MASKS[i] & NOT_H_FILE) {
                WHITE_PAWN_ATTACKSTO[i] |= SQUARE_MASKS[i-7];
            }
        }
        if (i <= 47) {
            // black pawns can't attack anything in ranks 7 or 8;
            if (SQUARE_MASKS[i] & NOT_A_FILE) {
                BLACK_PAWN_ATTACKSTO[i] |= SQUARE_MASKS[i+7];
            }
            if (SQUARE_MASKS[i] & NOT_H_FILE) {
                BLACK_PAWN_ATTACKSTO[i] |= SQUARE_MASKS[i+9];
            }
        }
    }
}

void const_bitmask_verify() {
    int i;

    uint_64 test, test2, test3;

    test = SQUARE_MASKS[A5] | SQUARE_MASKS[B5] | SQUARE_MASKS[C5] | SQUARE_MASKS[E5] | SQUARE_MASKS[F5] | SQUARE_MASKS[G5] | SQUARE_MASKS[H5];
    test |= (SQUARE_MASKS[D1] | SQUARE_MASKS[D2] | SQUARE_MASKS[D3] |SQUARE_MASKS[D4] | SQUARE_MASKS[D6] | SQUARE_MASKS[D7] |SQUARE_MASKS[D8]);

    printf("D5 Sliders - Test: %lx Actual: %lx\n", test, SLIDER_MOVES[D5]);

    test = (A_FILE | RANK_1) & (~SQUARE_MASKS[A1]);

    printf("A1 Sliders - Test: %lx  Actual: %lx\n", test, SLIDER_MOVES[A1]);

    test = SQUARE_MASKS[A1] | SQUARE_MASKS[C3] | SQUARE_MASKS[D4] | SQUARE_MASKS[E5] | SQUARE_MASKS [F6] | SQUARE_MASKS [G7] | SQUARE_MASKS[H8];
    test |= (SQUARE_MASKS[C1] | SQUARE_MASKS[A3]);

    printf("B2 Diagonals - Test: %lx  Actual: %lx\n", test, DIAGONAL_MOVES[B2]);

    test = SQUARE_MASKS[G3] | SQUARE_MASKS[F2] | SQUARE_MASKS[E1] | SQUARE_MASKS [G5] | SQUARE_MASKS[F6] | SQUARE_MASKS[E7] | SQUARE_MASKS[D8];
    printf("H4 Diagonals - Test: %lx  Actual: %lx\n", test, DIAGONAL_MOVES[H4]);

    test = SQUARE_MASKS[A1] | SQUARE_MASKS[B2] | SQUARE_MASKS[C3] | SQUARE_MASKS[E5] | SQUARE_MASKS[F6] | SQUARE_MASKS[G7] | SQUARE_MASKS[H8];
    printf("Test: %lx   B-File: %lx   Multiplied: %lx  Shift 58: %lx  again %ld\n",
           test, B_FILE, (test * B_FILE), (test * B_FILE) >> 58, (test * B_FILE) >> 58);

}

void code_generator()
{
    int i;

    printf ("const uint_64 A_FILE = 0x%lxul;\n", A_FILE);
    printf ("const uint_64 B_FILE = 0x%lxul;\n", B_FILE);
    printf ("const uint_64 G_FILE = 0x%lxul;\n", G_FILE);
    printf ("const uint_64 H_FILE = 0x%lxul;\n", H_FILE);
    printf ("const uint_64 RANK_1 = 0x%lxul;\n", RANK_1);
    printf ("const uint_64 RANK_2 = 0x%lxul;\n", RANK_2);
    printf ("const uint_64 RANK_7 = 0x%lxul;\n", RANK_7);
    printf ("const uint_64 RANK_8 = 0x%lxul;\n", RANK_8);
    printf ("const uint_64 NOT_A_FILE = 0x%lxul;\n", NOT_A_FILE);
    printf ("const uint_64 NOT_B_FILE = 0x%lxul;\n", NOT_B_FILE);
    printf ("const uint_64 NOT_G_FILE = 0x%lxul;\n", NOT_G_FILE);
    printf ("const uint_64 NOT_H_FILE = 0x%lxul;\n", NOT_H_FILE);
    printf ("const uint_64 NOT_RANK_1 = 0x%lxul;\n", NOT_RANK_1);
    printf ("const uint_64 NOT_RANK_2 = 0x%lxul;\n", NOT_RANK_2);
    printf ("const uint_64 NOT_RANK_7 = 0x%lxul;\n", NOT_RANK_7);
    printf ("const uint_64 NOT_RANK_8 = 0x%lxul;\n", NOT_RANK_8);
    printf ("const uint_64 ON_AN_EDGE = 0x%lxul;\n", ON_AN_EDGE);
    printf ("const uint_64 NOT_ANY_EDGE = 0x%lxul;\n", NOT_ANY_EDGE);

    printf ("\n\n");

    printf("const uint_64 SQUARE_MASKS[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", SQUARE_MASKS[i]);
    }
    printf ("0x%lxull\n};\n", SQUARE_MASKS[63]);


    printf ("\n\n");

    printf("const uint_64 NOT_MASKS[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", NOT_MASKS[i]);
    }
    printf ("0x%lxull\n};\n", NOT_MASKS[63]);

    printf ("\n\n");

    printf("const uint_64 KNIGHT_MOVES[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", KNIGHT_MOVES[i]);
    }
    printf ("0x%lxull\n};\n", KNIGHT_MOVES[63]);

    printf ("\n\n");

    printf("const uint_64 KING_MOVES[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", KING_MOVES[i]);
    }
    printf ("0x%lxull\n};\n", KING_MOVES[63]);

    printf ("\n\n");

    printf("const uint_64 SLIDER_MOVES[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", SLIDER_MOVES[i]);
    }
    printf ("0x%lxull\n};\n", SLIDER_MOVES[63]);

    printf ("\n\n");

    printf("const uint_64 DIAGONAL_MOVES[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", DIAGONAL_MOVES[i]);
    }
    printf ("0x%lxull\n};\n", DIAGONAL_MOVES[63]);

    printf("const uint_64 WHITE_PAWN_ATTACKSTO[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", WHITE_PAWN_ATTACKSTO[i]);
    }
    printf ("0x%lxull\n};\n", WHITE_PAWN_ATTACKSTO[63]);

    printf("const uint_64 BLACK_PAWN_ATTACKSTO[64] = {");
    for (i=0; i< 63; i++) {
        if (i%4 == 0) {
            printf ("\n");
        }
        printf("0x%lxull, ", BLACK_PAWN_ATTACKSTO[i]);
    }
    printf ("0x%lxull\n};\n", BLACK_PAWN_ATTACKSTO[63]);
}

void main(void)
{
    const_bitmask_init();
    code_generator();
}