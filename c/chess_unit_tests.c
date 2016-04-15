#include <stdio.h>
#include <string.h>
#include "chess_constants.h"
#include "chessmove.h"
#include "chessboard.h"

short gen_capture_differential(uc piece_moving, uc piece_captured)
{
    short pawn = 100;
    short bishop = 300;
    short knight = 290;
    short rook = 500;
    short queen = 900;
    short king = 20000;

    short pmval, pcval;

    if (piece_moving & PAWN) {
        pmval = pawn;
    } else if (piece_moving & BISHOP) {
        pmval = bishop;
    } else if (piece_moving & KNIGHT) {
        pmval = knight;
    } else if (piece_moving & ROOK) {
        pmval = rook;
    } else if (piece_moving & QUEEN) {
        pmval = queen;
    } else if (piece_moving & KING) {
        pmval = king;
    }

    if (piece_captured & PAWN) {
        pcval = pawn;
    } else if (piece_captured & BISHOP) {
        pcval = bishop;
    } else if (piece_captured & KNIGHT) {
        pcval = knight;
    } else if (piece_captured & ROOK) {
        pcval = rook;
    } else if (piece_captured & QUEEN) {
        pcval = queen;
    } else if (piece_captured & KING) {
        pcval = king;
    }

    return (pcval - pmval);
}

int move_tests()
{
    Move m;
    char *movestr;
    int success = 0;
    int fail = 0;
    uc startrank, startfile, endrank, endfile;

    square instart, inend, outstart, outend;
    uc inpiece_moving, inpiece_captured, inpromoted_to, inmove_flags;
    uc outpiece_moving, outpiece_captured, outpromoted_to, outmove_flags;
    short i, j, k, l, mf, incapture_differential, outcapture_differential;

    uc parray[13] = {0, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK};
    uc promolist[9] = {0, WN, WB, WR, WQ, BN, BB, BR, BQ};



    // Move create_move(square start, square end, uc piece_moving, uc piece_captured, short capture_differential, uc promoted_to, uc move_flags);
    // bool parse_move(Move move, square *pStart, square *pEnd, uc *pPiece_moving, uc *pPiece_captured, short *pCapture_differential, uc *pPromoted_to, uc *pMove_flags);

    // test move creation and parsing
    for (startrank = 20; startrank < 100; startrank = startrank + 10) {
        for (startfile = 1; startfile < 9; startfile ++) {
            for (endrank = 20; endrank < 100; endrank = endrank + 10) {
                for (endfile = 1; endfile < 9; endfile ++) {
                    if (startfile != endfile || startrank != endrank) {
                        for (i=0; i<13; i++) {
                            for (j=0; j<9;j++) {
                                for (mf = 0; mf < 16; mf ++) {
                                    for (k=0; k<13; k++) {
                                        instart = startrank + startfile;
                                        inend = endrank + endfile;
                                        inpiece_moving = parray[i];
                                        inpiece_captured = parray[k];
                                        if (inpiece_captured > 0) {
                                            incapture_differential = gen_capture_differential(inpiece_moving, inpiece_captured);
                                        } else {
                                            incapture_differential = 0;
                                        }
                                        inpromoted_to = promolist[j];
                                        m = create_move(instart, inend, inpiece_moving, inpiece_captured, incapture_differential,inpromoted_to,mf);
                                        parse_move(m, &outstart, &outend, &outpiece_moving, &outpiece_captured, &outcapture_differential, &outpromoted_to, &outmove_flags);
                                        movestr = pretty_print_move(m);
                                        if (instart != outstart) {
                                            printf("Mismatch: Move: %s, instart: %d, outstart %d\n", movestr, (short)instart, (short)outstart);
                                            fail++;
                                        } else if (inend != outend) {
                                            printf("Mismatch: Move: %s, inend: %d, outend %d\n", movestr, (short)inend, (short)outend);
                                            fail++;
                                        } else if (inpiece_moving != outpiece_moving) {
                                            printf("Mismatch: Move: %s, inpiece_moving: %d, outpiece_moving %d\n", movestr, (short)inpiece_moving, (short)outpiece_moving);
                                            fail++;
                                        } else if (inpiece_captured != outpiece_captured) {
                                            printf("Mismatch: Move: %s, inpiece_captured: %d, outpiece_captured %d\n", movestr, (short)inpiece_captured, (short)outpiece_captured);
                                            fail++;
                                        } else if (inpromoted_to != outpromoted_to) {
                                            printf("Mismatch: Move: %s, inpiece_promotedto: %d, outpiece_promotedto %d\n", movestr, (short)inpromoted_to, (short)outpromoted_to);
                                            fail++;
                                        }
                                        else if (mf != outmove_flags) {
                                            printf("Mismatch: Move: %s, inmove_flags: %d, outmove_flags %d\n", movestr, (short)mf, (short)outmove_flags);
                                            fail++;
                                        } else {
                                            success++;
                                        }
                                        free(movestr);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    printf("move generation tests result:  Success: %d,  Failure: %d\n", success, fail);

    success = 0;
    fail = 0;

    // move white pawn a7-a8, promote to rook
    m = create_move(81,91,WP,0,0,WR,0);
    movestr = pretty_print_move(m);
    if (strcmp (movestr, "a7-a8(R)") == 0) {
        success++;
    } else {
        printf("a7-a8(R) failed as %s\n", movestr);
        fail++;
    }
    free(movestr);

    // move black queen d8-d2, capture pawn, put king in check
    m = create_move(94, 34, BQ, WP, -800, 0, MOVE_CHECK);
    movestr = pretty_print_move(m);
    if (strcmp (movestr, "d8xd2+") == 0) {
        success++;
    } else {
        printf("d8xd2+ failed as %s\n", movestr);
        fail++;
    }
    free(movestr);



    printf("move_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int move_list_tests()
{
    struct MoveList *pList;
    struct MoveListNode *pMove;
    Move m;
    int success = 0;
    int fail = 0;


    pList = new_empty_move_list();
    m = create_move(21,31,WR,0,0,0,0);
    add_move_to_list(pList, m);

    m = create_move(95,97,BK,0,0,0,MOVE_CASTLE);
    add_move_to_list(pList, m);

    m = create_move(34,56,WB,BP,0,0,0);
    add_move_to_list(pList, m);

    pMove = pList->first;
    if (pMove == NULL) {
        printf("Corrupt Move list at spot #1\n");
        fail ++;
    } else {
        if (pMove->m == create_move(21, 31, WR, 0, 0, 0, 0)) {
            success++;
        } else {
            printf("First move in list was incorrect\n");
            fail++;
        }
        pMove = pMove -> next;
    }
    if (pMove == NULL) {
        printf("Corrupt Move list at spot #2\n");
        fail ++;
    } else {
        if (pMove->m == create_move(95,97,BK,0,0,0,MOVE_CASTLE)) {
            success++;
        } else {
            printf("Second move in list was incorrect\n");
            fail++;
        }
        pMove = pMove -> next;
    }
    if (pMove == NULL) {
        printf("Corrupt Move list at spot #3\n");
        fail ++;
    } else {
        if (pMove->m == create_move(34,56,WB,BP,0,0,0)) {
            if (pMove == pList->last) {
                success++;
            } else {
                printf("Corrupt 'Last' element in move list\n");
                fail++;
            }
        } else {
            printf("Third move in list was incorrect\n");
            fail++;
        }
        pMove = pMove -> next;
    }
    if (pMove != NULL) {
        printf("Corrupt 'Next' pointer in final node in list.");
    }

    printf("move_list_tests result:  Success: %d,  Failure: %d\n", success, fail);
    delete_moves_in_list(pList);
    free(pList);
    return(0);
}



int fen_tests()
{
    struct ChessBoard *pb;
    char *boardprint;
    int success = 0;
    int fail = 0;


    pb = new_board();
    erase_board(pb);

    if (!load_from_fen(pb, "k7/8/8/8/8/8/8/7K w - - 0 1")) {
        printf("Failed to load FEN #1\n");
        fail++;
    } else {
        boardprint = print_board(pb);
        if (strcmp(boardprint, "k.......\n........\n........\n........\n........\n........\n........\n.......K\n") != 0) {
            printf("Incorrect board print FEN #1:\n%s\n", boardprint);
            fail++;
        } else {
            if (pb-> halfmove_clock != 0 || pb-> ep_target != 0 || pb -> fullmove_number != 1 || pb->attrs != W_TO_MOVE) {
                printf("Incorrect attributes FEN #1\n");
                fail++;
            } else {
                success++;
            }
        }
        free(boardprint);
    }
    erase_board(pb);

    if (!load_from_fen(pb,"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 19")) {
        printf("Failed to load FEN #2\n");
        fail++;
    } else {
        boardprint = print_board(pb);
        if (strcmp(boardprint, "r...k..r\nPppp.ppp\n.b...nbN\nnP......\nBBP.P...\nq....N..\nPp.P..PP\nR..Q.RK.\n") != 0) {
            printf("Incorrect board print FEN #2:\n%s\n", boardprint);
            fail++;
        } else {
            if (pb->halfmove_clock != 0 || pb -> ep_target != 0 || pb -> fullmove_number != 19 || pb -> attrs != (W_TO_MOVE | B_CASTLE_KING | B_CASTLE_QUEEN)) {
                printf("Incorrect attributes FEN #2\n");
                fail ++;
            } else {
                success++;
            }
        }
        free(boardprint);
    }

    if (!load_from_fen(pb, "k7/8/8/8/P7/8/8/R3K2R b KQ a3 0 1")) {
        printf("Failed to load FEN #3\n");
        fail ++;
    } else {
        boardprint = print_board(pb);
        if (strcmp(boardprint, "k.......\n........\n........\n........\nP.......\n........\n........\nR...K..R\n") != 0) {
            printf("Incorrect board print FEN #3:\n%s\n", boardprint);
            fail++;
        } else {
            if (pb->halfmove_clock != 0 || pb -> ep_target != 41 || pb -> fullmove_number != 1 || pb -> attrs != (W_CASTLE_KING | W_CASTLE_QUEEN)) {
                printf("Incorrect attributes FEN #3\n");
                fail ++;
            } else {
                success++;
            }
        }
        free(boardprint);
    }

    free(pb);
    printf("fen_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}



int main() {
    move_tests();
    move_list_tests();
    fen_tests();
    return 0;
}