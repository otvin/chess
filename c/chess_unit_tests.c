#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess_constants.h"
#include "chessmove.h"
#include "chessboard.h"
#include "check_tables.h"
#include "generate_moves.h"

int gen_capture_differential(uc piece_moving, uc piece_captured)
{
    int pawn = 100;
    int bishop = 300;
    int knight = 290;
    int rook = 500;
    int queen = 900;
    int king = 20000;

    int pmval = 0, pcval = 0;

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

int move_tests(bool include_move_creation_test)
{
    Move m;
    char *movestr;
    int success = 0;
    int fail = 0;
    uc startrank, startfile, endrank, endfile;

    square instart, inend, outstart, outend;
    uc inpiece_moving, inpiece_captured, inpromoted_to, mf;
    uc outpiece_moving, outpiece_captured, outpromoted_to, outmove_flags;
    int i, j, k, incapture_differential, outcapture_differential;

    uc parray[13] = {0, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK};
    uc promolist[9] = {0, WN, WB, WR, WQ, BN, BB, BR, BQ};


    if (include_move_creation_test) {
        // test move creation and parsing
        for (startrank = 20; startrank < 100; startrank = startrank + 10) {
            for (startfile = 1; startfile < 9; startfile++) {
                for (endrank = 20; endrank < 100; endrank = endrank + 10) {
                    for (endfile = 1; endfile < 9; endfile++) {
                        if (startfile != endfile || startrank != endrank) {
                            for (i = 0; i < 13; i++) {
                                for (j = 0; j < 9; j++) {
                                    for (mf = 0; mf < 16; mf++) {
                                        for (k = 0; k < 13; k++) {
                                            instart = startrank + startfile;
                                            inend = endrank + endfile;
                                            inpiece_moving = parray[i];
                                            inpiece_captured = parray[k];
                                            if (inpiece_captured > 0) {
                                                incapture_differential = gen_capture_differential(inpiece_moving,
                                                                                                  inpiece_captured);
                                            } else {
                                                incapture_differential = 0;
                                            }
                                            inpromoted_to = promolist[j];
                                            m = create_move(instart, inend, inpiece_moving, inpiece_captured,
                                                            incapture_differential, inpromoted_to, mf);
                                            parse_move(m, &outstart, &outend, &outpiece_moving, &outpiece_captured,
                                                       &outcapture_differential, &outpromoted_to, &outmove_flags);
                                            movestr = pretty_print_move(m);
                                            if (instart != outstart) {
                                                printf("Mismatch: Move: %s, instart: %d, outstart %d\n", movestr,
                                                       (int) instart, (int) outstart);
                                                fail++;
                                            } else if (inend != outend) {
                                                printf("Mismatch: Move: %s, inend: %d, outend %d\n", movestr,
                                                       (int) inend, (int) outend);
                                                fail++;
                                            } else if (inpiece_moving != outpiece_moving) {
                                                printf("Mismatch: Move: %s, inpiece_moving: %d, outpiece_moving %d\n",
                                                       movestr, (int) inpiece_moving, (int) outpiece_moving);
                                                fail++;
                                            } else if (inpiece_captured != outpiece_captured) {
                                                printf("Mismatch: Move: %s, inpiece_captured: %d, outpiece_captured %d\n",
                                                       movestr, (int) inpiece_captured, (int) outpiece_captured);
                                                fail++;
                                            } else if (inpromoted_to != outpromoted_to) {
                                                printf("Mismatch: Move: %s, inpiece_promotedto: %d, outpiece_promotedto %d\n",
                                                       movestr, (int) inpromoted_to, (int) outpromoted_to);
                                                fail++;
                                            }
                                            else if (mf != outmove_flags) {
                                                printf("Mismatch: Move: %s, inmove_flags: %d, outmove_flags %d\n",
                                                       movestr, mf, (int) outmove_flags);
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
    }
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
    struct MoveList ml;
    Move m;
    int success = 0;
    int fail = 0;

    MOVELIST_CLEAR(&ml);
    m = create_move(21,31,WR,0,0,0,0);
    MOVELIST_ADD(&ml, m);

    m = create_move(95,97,BK,0,0,0,MOVE_CASTLE);
    MOVELIST_ADD(&ml, m);

    m = create_move(34,56,WB,BP,0,0,0);
    MOVELIST_ADD(&ml, m);

    if (ml.size != 3) {
        printf("Invalid list size %d\n", ml.size);
        fail ++;
    }

    if (ml.moves[0] == create_move(21, 31, WR, 0, 0, 0, 0)) {
        success++;
    } else {
        printf("First move in list was incorrect\n");
        fail++;
    }

    if (ml.moves[1] == create_move(95,97,BK,0,0,0,MOVE_CASTLE)) {
        success++;
    } else {
        printf("Second move in list was incorrect\n");
        fail++;
    }

    if (ml.moves[2] == create_move(34,56,WB,BP,0,0,0)) {
        success++;
    } else {
        printf("Third move in list was incorrect\n");
        fail++;
    }

    printf("move_list_tests result:  Success: %d,  Failure: %d\n", success, fail);
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

int apply_move_tests()
{
    struct ChessBoard *pb;
    Move m;
    char *boardprint;
    int success = 0;
    int fail = 0;

    pb = new_board();
    set_start_position(pb);
    m = create_move(34, 54, WP, 0, 0, 0, MOVE_DOUBLE_PAWN);
    apply_move(pb, m);
    boardprint = print_board(pb);
    if (strcmp(boardprint, "rnbqkbnr\npppppppp\n........\n........\n...P....\n........\nPPP.PPPP\nRNBQKBNR\n") != 0) {
        printf("Incorrect board print apply move #1:\n%s\n", boardprint);
        fail ++;
    } else {
        if (pb->halfmove_clock != 0 || pb -> fullmove_number != 1 || pb -> attrs != (W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN) || pb -> ep_target != 44) {
            printf("Incorrect atributes apply move #1\n");
            fail ++;
        } else {
            success++;
        }
    }
    free(boardprint);

    free(pb);
    printf("apply_move_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}


bool test_a_check(const char* fen, const char* position_name, bool supposed_to_be_check)
{
    struct ChessBoard *pb;
    bool ret;

    pb = new_board();
    if (!load_from_fen(pb, fen)) {
        printf("Invalid FEN %s in test a check %s\n", fen, position_name);
        ret = false;
    } else {
        if (side_to_move_is_in_check(pb) == supposed_to_be_check) {
            ret = true;
        } else {
            printf("Test check %s failed\n", position_name);
            ret = false;
        }
    }
    free(pb);
    return ret;
}

int check_tests() {
    int success = 0;
    int fail = 0;
    test_a_check("K7/8/8/Q7/8/7k/8/8 w - - 1 1", "Q1", false) ? success++ : fail++;
    test_a_check("k7/8/8/Q7/8/7K/8/8 w - - 1 1", "Q2", false) ? success++ : fail++;
    test_a_check("k7/8/8/Q7/8/7K/8/8 b - - 1 1", "Q3", true) ? success++ : fail++;
    test_a_check("k7/p7/8/Q7/8/7K/8/8 b - - 1 1", "Q4", false) ? success++ : fail++;
    test_a_check("k7/p7/8/3Q4/8/7K/8/8 b - - 1 1", "Q5", true) ? success++ : fail++;

    test_a_check("k7/p7/8/3B4/8/7K/8/8 b - - 1 1", "B1", true) ? success++ : fail++;
    test_a_check("k7/p7/8/3b4/8/7K/8/8 b - - 1 1", "B2", false) ? success++ : fail++;
    test_a_check("k7/pp6/8/3B4/8/7K/8/8 b - - 1 1", "B3", false) ? success++ : fail++;

    test_a_check("K7/8/8/R7/8/7k/8/8 w - - 1 1", "R1", false) ? success++ : fail++;
    test_a_check("k7/8/8/R7/8/7K/8/8 w - - 1 1", "R2", false) ? success++ : fail++;
    test_a_check("k7/8/8/R7/8/7K/8/8 b - - 1 1", "R3", true) ? success++ : fail++;

    test_a_check("k7/pP6/8/3b4/8/7K/8/8 b - - 1 1", "P1", true) ? success++ : fail++;
    test_a_check("1k6/Pp6/8/3b4/8/7K/8/8 b - - 1 1", "P2", true) ? success++ : fail++;
    test_a_check("4N3/5P1P/5N1k/Q5p1/5PKP/B7/8/1B6 w - - 0 1", "P3", false) ? success++ : fail++;
    test_a_check("4N3/5P1P/5N1k/Q5P1/6KP/B7/8/1B6 b - - 0 1", "P4", true) ? success++ : fail++;

    test_a_check("k7/2N5/8/8/8/8/8/K7 b - - 1 1", "N1", true) ? success++ : fail++;

    test_a_check("kK6/8/8/8/8/8/8/8 b - - 1 1 ", "K1", true) ? success++ : fail++;
    test_a_check("kK6/8/8/8/8/8/8/8 w - - 1 1 ", "K2", true) ? success++ : fail++;

    printf("check_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int macro_tests()
{
    int success = 0, fail = 0;

    if (OPPOSITE_COLORS(WN,BP)) {
        success++;
    } else {
        printf("Opposite WN/BP failed.\n");
        fail++;
    }
    if (OPPOSITE_COLORS(BQ,BK)) {
        fail++;
    } else {
        success++;
    }
    if (OPPOSITE_COLORS(WP,WR)) {
        printf("Opposite WP/WR failed.\n");
        fail++;
    } else {
        success++;
    }

    if (SAME_COLORS(BR,BB)) {
        success++;
    } else {
        printf("Same BR/BB failed.\n");
        fail++;
    }

    if (SAME_COLORS(WK, WN)) {
        success++;
    } else {
        printf("Same WK/WN failed.\n");
        fail++;
    }

    if (SAME_COLORS(BN, WP)) {
        printf("Same BN/WP failed.\n");
        fail++;
    } else {
        success++;
    }

    printf("macro_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int move_generation_tests()
{
    int success = 0, fail = 0;
    struct ChessBoard *pb;
    struct MoveList ml;

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "k7/8/8/8/b3Q2P/3p4/8/7K w - - 0 1");
    printf("%s\n",print_board(pb));
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "k7/8/8/2n5/b3Q2P/3p4/8/7K b - - 0 1");
    printf("%s\n",print_board(pb));
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);


    return 0;
}


int main() {

    init_check_tables();

    move_tests(false);
    move_list_tests();
    fen_tests();
    apply_move_tests();
    check_tests();
    macro_tests();
    move_generation_tests();
    return 0;
}