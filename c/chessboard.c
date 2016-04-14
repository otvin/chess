
#include "chessboard.h"


bool arraypos_is_on_board(square pos) {
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


void erase_board(struct ChessBoard *pb) {
    int i;

    for (i=0; i<120; i++) {
        if (arraypos_is_on_board(i)) {
            pb->squares[i] = EMPTY;
        }
        else {
            pb->squares[i] = OFF_BOARD;
        }
    }
    pb->ep_target = 0;
}

void set_start_position(struct ChessBoard *pb) {
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
}

int print_board(struct ChessBoard *pb) {
    int i;
    int j;

    for (i = 90; i > 10; i = i-10) {
        for (j = 1; j < 9; j++) {
            switch(pb->squares[i+j]){
                case(WP):
                    printf("P");
                    break;
                case(WN):
                    printf("N");
                    break;
                case(WB):
                    printf("B");
                    break;
                case(WR):
                    printf("R");
                    break;
                case(WQ):
                    printf("Q");
                    break;
                case(WK):
                    printf("K");
                    break;
                case(BP):
                    printf("p");
                    break;
                case(BN):
                    printf("n");
                    break;
                case(BB):
                    printf("b");
                    break;
                case(BR):
                    printf("r");
                    break;
                case(BQ):
                    printf("q");
                    break;
                case(BK):
                    printf("k");
                    break;
                case(EMPTY):
                    printf(".");
                    break;
                default:
                    printf("X");
            }
        }
        printf("\n");
    }


}


struct ChessBoard *new_board() {
    struct ChessBoard *ret;
    ret = (ChessBoard *)malloc(sizeof(ChessBoard));
    return(ret);
}
