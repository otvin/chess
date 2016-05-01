#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "chess_constants.h"
#include "chessmove.h"
#include "chessboard.h"
#include "generate_moves.h"
#include "evaluate_board.h"
#include "hash.h"

void movelist_sort_alpha(struct MoveList *ml, bool is_classic)
{
    int i, j;
    Move swap;
    char *m1, *m2;

    for (i=0; i < (ml->size) -1 ; i++) {
        for (j=i+1; j < ml-> size; j++) {
            if (is_classic) {
                m1 = pretty_print_move(ml->moves[i]);
                m2 = pretty_print_move(ml->moves[j]);
            } else {
                m1 = pretty_print_bb_move(ml->moves[i]);
                m2 = pretty_print_bb_move(ml->moves[j]);
            }
            if (strcmp(m1,m2) > 0) {
                swap = ml->moves[j];
                ml->moves[j] = ml->moves[i];
                ml->moves[i] = swap;
            }
            free(m1);
            free(m2);
        }
    }
}

int gen_capture_differential(uc piece_moving, uc piece_captured)
{
    return(piece_value(piece_captured) - piece_value(piece_moving));
}

int move_tests(bool include_move_creation_test, int *s, int *f)
{
    Move m, m2;
    char *movestr;
    int success = 0;
    int fail = 0;
    uc startrank, startfile, endrank, endfile;

    uc instart, inend, outstart, outend;
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

                                            inpromoted_to = promolist[j];
                                            m = create_move(instart, inend, inpiece_moving, inpiece_captured,
                                                            inpromoted_to, mf);
                                            m2 = CREATE_MOVE(instart, inend, inpiece_moving, inpiece_captured, inpromoted_to, mf);
                                            movestr = pretty_print_move(m);
                                            if (m!=m2) {
                                                printf("Original: %ld,  Macro: %ld,  move=%s\n", m, m2, movestr);
                                                fail++;
                                                *s = *s + success;
                                                *f = *f + fail;
                                                return 0;
                                            }

                                            parse_move(m, &outstart, &outend, &outpiece_moving, &outpiece_captured,
                                                       &outpromoted_to, &outmove_flags);

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
    m = create_move(81,91,WP,0,WR,0);
    movestr = pretty_print_move(m);
    if (strcmp (movestr, "a7-a8(R)") == 0) {
        success++;
    } else {
        printf("a7-a8(R) failed as %s\n", movestr);
        fail++;
    }
    free(movestr);

    // move black queen d8-d2, capture pawn, put king in check
    m = create_move(94, 34, BQ, WP, 0, MOVE_CHECK);
    movestr = pretty_print_move(m);
    if (strcmp (movestr, "d8xd2+") == 0) {
        success++;
    } else {
        printf("d8xd2+ failed as %s\n", movestr);
        fail++;
    }
    free(movestr);


    *s = *s + success;
    *f = *f + fail;
    printf("move_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int list_tests(int *s, int *f)
{
    struct MoveList ml;
    struct SquareList sl;
    Move m;
    int success = 0;
    int fail = 0;

    MOVELIST_CLEAR(&ml);
    m = create_move(21,31,WR,0,0,0);
    MOVELIST_ADD(&ml, m);

    m = create_move(95,97,BK,0,0,MOVE_CASTLE);
    MOVELIST_ADD(&ml, m);

    m = create_move(34,56,WB,BP,0,0);
    MOVELIST_ADD(&ml, m);

    if (ml.size != 3) {
        printf("Invalid list size %d\n", ml.size);
        fail ++;
    }

    if (ml.moves[0] == create_move(21, 31, WR, 0, 0, 0)) {
        success++;
    } else {
        printf("First move in list was incorrect\n");
        fail++;
    }

    if (ml.moves[1] == create_move(95,97,BK,0,0,MOVE_CASTLE)) {
        success++;
    } else {
        printf("Second move in list was incorrect\n");
        fail++;
    }

    if (ml.moves[2] == create_move(34,56,WB,BP,0,0)) {
        success++;
    } else {
        printf("Third move in list was incorrect\n");
        fail++;
    }

    movelist_remove(&ml, 1);

    if (ml.size != 2) {
        printf("Invalid list size %d\n", ml.size);
        fail ++;
    }

    if (ml.moves[0] == create_move(21, 31, WR, 0, 0, 0)) {
        success++;
    } else {
        printf("Post-delete: first move in list was incorrect\n");
        fail++;
    }

    if (ml.moves[1] == create_move(34,56,WB,BP,0, 0)) {
        success++;
    } else {
        printf("Post-delete: Second move in list was incorrect\n");
        fail++;
    }


    SQUARELIST_CLEAR(&sl);
    SQUARELIST_ADD(&sl, (uc)22);
    SQUARELIST_ADD(&sl, (uc)39);
    SQUARELIST_ADD(&sl, (uc)87);

    if (sl.size!=3) {
        printf("Invalid list size %d\n", sl.size);
        fail++;
    }

    if (sl.squares[0] == 22) {
        success ++;
    } else {
        printf("first square failed: %d instead of 22\n",sl.squares[0]);
        fail++;
    }

    if (sl.squares[1] == 39) {
        success++;
    } else {
        printf("second square failed: %d instead of 39\n", sl.squares[1]);
        fail++;
    }

    if (sl.squares[2] == 87) {
        success++;
    } else {
        printf("third square failed: %d instead of 87\n", sl.squares[2]);
    }

    squarelist_remove(&sl, 0);

    if (sl.size!=2) {
        printf("Invalid square list size %d\n", sl.size);
        fail++;
    }

    if (sl.squares[0] == 39) {
        success++;
    } else {
        printf("Post-delete: first square failed with %d instead of 39\n", sl.squares[0]);
        fail++;
    }

    if (sl.squares[1] == 87) {
        success++;
    } else {
        printf("Post-delete: second square failed with %d instead of 87\n", sl.squares[1]);
        fail++;
    }

    *s = *s + success;
    *f = *f + fail;
    printf("list_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return(0);
}



int fen_tests(int *s, int *f)
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
                printf("Incorrect attributes FEN #1: halfmove clock = %d, ep_target = %d, fullmove number = %d, attrs = %d\n", pb->halfmove_clock, pb->ep_target, pb->fullmove_number, pb->attrs);
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
            if (pb->halfmove_clock != 0 || pb -> ep_target != 0 || pb -> fullmove_number != 19 || pb -> attrs != (W_TO_MOVE | B_CASTLE_KING | B_CASTLE_QUEEN | BOARD_IN_CHECK)) {
                printf("Incorrect attributes FEN #2: halfmove clock = %d, ep target = %d, fullmove number = %d, attrs = %d\n", pb->halfmove_clock, pb->ep_target, pb->fullmove_number, pb->attrs);
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
    *s = *s + success;
    *f = *f + fail;
    printf("fen_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int apply_move_tests(int *s, int *f)
{
    struct ChessBoard *pb;
    Move m;
    char *boardprint;
    int success = 0;
    int fail = 0;

    pb = new_board();
    set_start_position(pb);
    m = create_move(34, 54, WP, 0, 0, MOVE_DOUBLE_PAWN);
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
    *s = *s + success;
    *f = *f + fail;
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
        if (side_to_move_is_in_check(pb, 0) == supposed_to_be_check) {
            ret = true;
        } else {
            printf("Test check %s failed\n", position_name);
            ret = false;
        }
    }
    free(pb);
    return ret;
}

int check_tests(int *s, int *f) {
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

    *s = *s + success;
    *f = *f + fail;
    printf("check_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int macro_tests(int *s, int*f)
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

    *s = *s + success;
    *f = *f + fail;
    printf("macro_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int printable_move_generation_tests()
{
    int success = 0, fail = 0;
    struct ChessBoard *pb;
    struct MoveList ml;
    char *boardprint;

    printf("\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "r3k2r/p1ppqpb1/bn2pnp1/1B1PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

/*
    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "k7/8/8/8/1Q6/8/8/7K w - - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);
*/

    /*
    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);




    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "k7/8/8/8/b3Q2P/3p4/8/7K w - - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "k7/8/8/2n5/b3Q2P/3p4/8/7K b - - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    set_start_position(pb);
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "r2qk2r/8/8/8/8/8/8/R2QK2R w kqKQ - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "5k2/8/8/8/8/8/8/R2QK2R w kqKQ - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);

    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);


    printf("\n\n\n\n");

    MOVELIST_CLEAR(&ml);
    pb = new_board();
    load_from_fen(pb, "8/PPPk4/8/8/8/8/4K1p1/5N1N b - - 0 1");
    boardprint = print_board(pb);
    printf("%s\n",boardprint);
    free(boardprint);
    generate_move_list(pb, &ml);
    print_move_list(&ml);
    free(pb);
    */

    return 0;
}




static long perft_counts[10];
typedef struct perft_list {
    int v[10];
} perft_list;


void calc_moves_classic(const struct ChessBoard *pb, int depth, bool divide)
{
    struct MoveList ml;
    struct ChessBoard tmp;
    int i,q;
    int x;
    char *s;
    long divide_array[10] = {0,0,0,0,0,0,0,0,0,0};
    long a;


    if (depth == 0) {
        perft_counts[0] ++;
        return;
    }


    MOVELIST_CLEAR(&ml);

#ifndef DISABLE_HASH
    if (!TT_probe(pb, &ml)) {
#endif
        generate_move_list(pb, &ml);
#ifndef DISABLE_HASH
        TT_insert(pb, &ml);
    }
#endif


    if (divide) {
        printf("\n\n");
        movelist_sort_alpha(&ml, true);
    }



    for (i=0; i<ml.size; i++) {
        tmp = *pb;

        if (divide) {
            for (q=0;q<10;q++) {
                divide_array[q] = perft_counts[q];
            }
            s = pretty_print_move(ml.moves[i]);
            printf("%s:\n",s);
            free(s);
        }
        perft_counts[depth] ++;
        apply_move(&tmp, ml.moves[i]);
        calc_moves_classic(&tmp, depth-1, false);
        if (divide) {
            for (q=depth; q>0; q--) {
                a = perft_counts[q] - divide_array[q];
                printf("\t depth:%d  num moves:%ld\n", depth-q, a);
            }
        }
    }
}



void calc_moves_bitboard(const struct bitChessBoard *pbb, int depth, bool divide)
{
    struct MoveList ml;
    struct bitChessBoard tmp;
    int i,q;
    int x;
    char *s;
    long divide_array[10] = {0,0,0,0,0,0,0,0,0,0};
    long a;


    if (depth == 0) {
        perft_counts[0] ++;
        return;
    }



    MOVELIST_CLEAR(&ml);

#ifndef DISABLE_HASH
    if (!TT_probe_bb(pbb, &ml)) {
#endif
        generate_bb_move_list(pbb, &ml);
#ifndef DISABLE_HASH
        TT_insert_bb(pbb, &ml);
    }
#endif


    if (divide) {
        printf("\n\n");
        movelist_sort_alpha(&ml, false);
    }

    for (i=0; i<ml.size; i++) {
        tmp = *pbb;

        if (divide) {
            for (q=0;q<10;q++) {
                divide_array[q] = perft_counts[q];
            }
            s = pretty_print_bb_move(ml.moves[i]);
            printf("%s:\n",s);
            free(s);
        }
        perft_counts[depth] ++;
        apply_bb_move(&tmp, ml.moves[i]);


        calc_moves_bitboard(&tmp, depth - 1, false);
        if (divide) {
            for (q = depth; q > 0; q--) {
                a = perft_counts[q] - divide_array[q];
                printf("\t depth:%d  num moves:%ld\n", depth - q, a);
            }
        }

    }
}



bool perft_classic(char *fen, int depth, struct perft_list pl, bool divide, bool times_only)
{
    struct ChessBoard *pb;
    clock_t start, stop;
    double elapsed, knps;
    bool ret;
    int i;
    long a,b, totalnodes = 0;

    pb = new_board();
    if (!load_from_fen(pb, fen)) {
        printf("Invalid FEN %s in perft \n", fen);
        ret = false;
    }
    memset(perft_counts, 0, sizeof(perft_counts));

    printf("\nPerft: %s\n", fen);
    start = clock();
    calc_moves_classic(pb, depth, divide);

    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    free(pb);

    ret = true;
    for (i=depth;i>0;i--) {
        a = perft_counts[i];
        totalnodes = totalnodes + a;
        b = pl.v[depth-i];
        if (a==b) {
            if (!times_only) {
                printf("Depth:%d nodes:%ld\n", depth - i + 1, a);
            }
        } else {
            printf("FAILED Depth:%d nodes:%ld expected nodes:%ld\n", depth-i+1, a, b);
            ret = false;
        }
    }
    printf("Time elapsed in ms: %f  Total nodes: %ld   kNPS = %f\n\n", elapsed, totalnodes, ((1.0 * totalnodes) / elapsed));
    return ret;
}

#ifdef _MSC_VER
#include<intrin.h>
#pragma intrinsic(__rdtsc())
static inline uint_64 rdtsc() {
	return __rdtsc();
}
#else
static inline uint_64 rdtsc() {
    // copyright (C) AJ Siemelink
    unsigned int hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((((uint_64)(hi))<<32)|lo);
}
#endif

bool perft_bitboard(char *fen, int depth, struct perft_list pl, bool divide, bool times_only)
{
    struct bitChessBoard *pbb;
    clock_t start, stop;
    double elapsed, knps;
    bool ret;
    int i;
    long a,b, totalnodes = 0;

    // the code to compute ticks per second came from FRC Perft, Copyright (C) 2008-2011 AJ Siemelink.  I converted
    // from C++ to C.  He notes that cpucycles may be inaccurate on multicore systems and suggests you Google RDTSC if you have questions.
    uint_64 _startticks;
    uint_64 _stopticks;
    double ticks;

    pbb = new_bitboard();
    if (!load_bitboard_from_fen(pbb, fen)) {
        printf("Invalid FEN %s in perft \n", fen);
        ret = false;
    }
    memset(perft_counts, 0, sizeof(perft_counts));

    printf("\nBitboard Perft: %s\n", fen);
    start = clock();
    _startticks = rdtsc();

    calc_moves_bitboard(pbb, depth, divide);

    _stopticks = rdtsc();
    stop = clock();


    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    ticks = (_stopticks - _startticks);

    free(pbb);

    ret = true;
    for (i=depth;i>0;i--) {
        a = perft_counts[i];
        totalnodes = totalnodes + a;
        b = pl.v[depth-i];
        if (a==b) {
            if (!times_only) {
                printf("Depth:%d nodes:%ld\n", depth - i + 1, a);
            }
        } else {
            printf("FAILED Depth:%d nodes:%ld expected nodes:%ld\n", depth-i+1, a, b);
            ret = false;
        }
    }
    printf("Time elapsed in ms: %f  Total nodes: %ld   \nkNPS = %f,  Ticks/operation: %6.1f\n\n", elapsed, totalnodes, ((1.0 * totalnodes) / elapsed), ticks/(double)totalnodes);



    return ret;
}

bool perft(char *fen, int depth, struct perft_list pl, bool divide, bool use_classic, bool times_only)
{
    if (use_classic) {
        return perft_classic(fen, depth, pl, divide, times_only);
    } else {
        return perft_bitboard(fen, depth, pl, divide, times_only);
    }
}


int perft_tests(bool include_extended_list, int *s, int *f, bool divide, bool use_classic, bool times_only)
{
    int success = 0;
    int fail = 0;

    // Copied from https://chessprogramming.wikispaces.com/Perft+Results
    perft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, (perft_list){20, 400, 8902, 197281, 4865609,119060324}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, (perft_list){48, 2039, 97862, 4085603}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6, (perft_list){14, 191, 2812, 43238, 674624, 11030083}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, (perft_list) {6, 264, 9467, 422333, 15833292}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 5, (perft_list) {6, 264, 9467, 422333, 15833292}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ", 5, (perft_list){44, 1486, 62379, 2103487, 89941194}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, (perft_list){46, 2079, 89890, 3894594}, divide, use_classic, times_only) ? success++ : fail ++;

    // Copied from https://github.com/thomasahle/sunfish/blob/master/tests/queen.fen
    perft("r1b2rk1/2p2ppp/p7/1p6/3P3q/1BP3bP/PP3QP1/RNB1R1K1 w - - 1 0", 4, (perft_list){40,1334,50182,1807137}, divide, use_classic, times_only) ? success++ : fail ++;

    // Copied from http://www.talkchess.com/forum/viewtopic.php?t=49118
    // perft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, (perft_list){46, 2079, 89890, 3894594, 164075551}, divide, use_classic, times_only) ? success++ : fail ++;
    perft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, (perft_list){46, 2079, 89890, 3894594}, divide, use_classic, times_only) ? success++ : fail ++;

    // Copied from http://www.rocechess.ch/perft.html
    perft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 5, (perft_list) {24, 496, 9483, 182838, 3605103}, divide, use_classic, times_only) ? success++ : fail ++;

    if (include_extended_list) {


        perft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 6, (perft_list) { 15 , 66 ,1197 , 7059 , 133987 , 764643}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, (perft_list) { 16 , 71 ,1287 , 7626 , 145232 , 846648}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 6, (perft_list) { 5 , 75 ,459 , 8290 , 47635 , 899442}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 6, (perft_list) { 5 , 80 ,493 , 8897 , 52710 , 1001523}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 6, (perft_list) { 26 , 112 ,3189 , 17945 , 532933 , 2788982}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 6, (perft_list) { 5 , 130 ,782 , 22180 , 118882 , 3517770}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 6, (perft_list) { 12 , 38 ,564 , 2219 , 37735 , 185867}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 6, (perft_list) { 15 , 65 ,1018 , 4573 , 80619 , 413018}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 6, (perft_list) { 3 , 32 ,134 , 2073 , 10485 , 179869}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 6, (perft_list) { 4 , 49 ,243 , 3991 , 20780 , 367724}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 6, (perft_list) { 26 , 568 ,13744 , 314346 , 7594526 , 179862938}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 6, (perft_list) { 25 , 567 ,14095 , 328965 , 8153719 , 195629489}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 6, (perft_list) { 25 , 548 ,13502 , 312835 , 7736373 , 184411439}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 6, (perft_list) { 25 , 547 ,13579 , 316214 , 7878456 , 189224276}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 6, (perft_list) { 26 , 583 ,14252 , 334705 , 8198901 , 198328929}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 6, (perft_list) { 25 , 560 ,13592 , 317324 , 7710115 , 185959088}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 6, (perft_list) { 25 , 560 ,13607 , 320792 , 7848606 , 190755813}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 6, (perft_list) { 5 , 75 ,459 , 8290 , 47635 , 899442}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 6, (perft_list) { 5 , 80 ,493 , 8897 , 52710 , 1001523}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 6, (perft_list) { 15 , 66 ,1197 , 7059 , 133987 , 764643}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 6, (perft_list) { 16 , 71 ,1287 , 7626 , 145232 , 846648}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 6, (perft_list) { 5 , 130 ,782 , 22180 , 118882 , 3517770}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 6, (perft_list) { 26 , 112 ,3189 , 17945 , 532933 , 2788982}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 6, (perft_list) { 3 , 32 ,134 , 2073 , 10485 , 179869}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 6, (perft_list) { 4 , 49 ,243 , 3991 , 20780 , 367724}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 6, (perft_list) { 12 , 38 ,564 , 2219 , 37735 , 185867}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 6, (perft_list) { 15 , 65 ,1018 , 4573 , 80619 , 413018}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 6, (perft_list) { 26 , 568 ,13744 , 314346 , 7594526 , 179862938}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 6, (perft_list) { 26 , 583 ,14252 , 334705 , 8198901 , 198328929}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 6, (perft_list) { 25 , 560 ,13592 , 317324 , 7710115 , 185959088}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 6, (perft_list) { 25 , 560 ,13607 , 320792 , 7848606 , 190755813}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 6, (perft_list) { 25 , 567 ,14095 , 328965 , 8153719 , 195629489}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 6, (perft_list) { 25 , 548 ,13502 , 312835 , 7736373 , 184411439}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 6, (perft_list) { 25 , 547 ,13579 , 316214 , 7878456 , 189224276}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 6, (perft_list) { 14 , 195 ,2760 , 38675 , 570726 , 8107539}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 6, (perft_list) { 11 , 156 ,1636 , 20534 , 223507 , 2594412}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 6, (perft_list) { 19 , 289 ,4442 , 73584 , 1198299 , 19870403}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 6, (perft_list) { 3 , 51 ,345 , 5301 , 38348 , 588695}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 6, (perft_list) { 17 , 54 ,835 , 5910 , 92250 , 688780}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 6, (perft_list) { 15 , 193 ,2816 , 40039 , 582642 , 8503277}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 6, (perft_list) { 16 , 180 ,2290 , 24640 , 288141 , 3147566}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 6, (perft_list) { 4 , 68 ,1118 , 16199 , 281190 , 4405103}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 6, (perft_list) { 17 , 54 ,835 , 5910 , 92250 , 688780}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 6, (perft_list) { 3 , 51 ,345 , 5301 , 38348 , 588695}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 6, (perft_list) { 17 , 278 ,4607 , 76778 , 1320507 , 22823890}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 6, (perft_list) { 21 , 316 ,5744 , 93338 , 1713368 , 28861171}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 6, (perft_list) { 21 , 144 ,3242 , 32955 , 787524 , 7881673}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 6, (perft_list) { 7 , 143 ,1416 , 31787 , 310862 , 7382896}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 6, (perft_list) { 6 , 106 ,1829 , 31151 , 530585 , 9250746}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 6, (perft_list) { 17 , 309 ,5133 , 93603 , 1591064 , 29027891}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 6, (perft_list) { 7 , 143 ,1416 , 31787 , 310862 , 7382896}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 6, (perft_list) { 21 , 144 ,3242 , 32955 , 787524 , 7881673}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 6, (perft_list) { 19 , 275 ,5300 , 104342 , 2161211 , 44956585}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 6, (perft_list) { 36 , 1027 ,29215 , 771461 , 20506480 , 525169084}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 6, (perft_list) { 19 , 275 ,5300 , 104342 , 2161211 , 44956585}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 6, (perft_list) { 36 , 1027 ,29227 , 771368 , 20521342 , 524966748}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("6kq/8/8/8/8/8/8/7K w - - 0 1", 6, (perft_list) { 2 , 36 ,143 , 3637 , 14893 , 391507}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 6, (perft_list) { 2 , 36 ,143 , 3637 , 14893 , 391507}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 6, (perft_list) { 6 , 35 ,495 , 8349 , 166741 , 3370175}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("6qk/8/8/8/8/8/8/7K b - - 0 1", 6, (perft_list) { 22 , 43 ,1015 , 4167 , 105749 , 419369}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 6, (perft_list) { 2 , 36 ,143 , 3637 , 14893 , 391507}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 6, (perft_list) { 6 , 35 ,495 , 8349 , 166741 , 3370175}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 6, (perft_list) { 3 , 7 ,43 , 199 , 1347 , 6249}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/7K/7P/7k w - - 0 1", 6, (perft_list) { 3 , 7 ,43 , 199 , 1347 , 6249}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 6, (perft_list) { 1 , 3 ,12 , 80 , 342 , 2343}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 6, (perft_list) { 1 , 3 ,12 , 80 , 342 , 2343}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 6, (perft_list) { 7 , 35 ,210 , 1091 , 7028 , 34834}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 6, (perft_list) { 1 , 3 ,12 , 80 , 342 , 2343}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/7K/7P/7k b - - 0 1", 6, (perft_list) { 1 , 3 ,12 , 80 , 342 , 2343}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 6, (perft_list) { 3 , 7 ,43 , 199 , 1347 , 6249}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 6, (perft_list) { 3 , 7 ,43 , 199 , 1347 , 6249}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 6, (perft_list) { 5 , 35 ,182 , 1091 , 5408 , 34822}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 6, (perft_list) { 2 , 8 ,44 , 282 , 1814 , 11848}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 6, (perft_list) { 2 , 8 ,44 , 282 , 1814 , 11848}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 6, (perft_list) { 3 , 9 ,57 , 360 , 1969 , 10724}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 6, (perft_list) { 3 , 9 ,57 , 360 , 1969 , 10724}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 6, (perft_list) { 5 , 25 ,180 , 1294 , 8296 , 53138}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 6, (perft_list) { 8 , 61 ,483 , 3213 , 23599 , 157093}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 6, (perft_list) { 8 , 61 ,411 , 3213 , 21637 , 158065}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 6, (perft_list) { 4 , 15 ,90 , 534 , 3450 , 20960}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 6, (perft_list) { 3 , 9 ,57 , 360 , 1969 , 10724}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 6, (perft_list) { 3 , 9 ,57 , 360 , 1969 , 10724}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 6, (perft_list) { 5 , 25 ,180 , 1294 , 8296 , 53138}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 6, (perft_list) { 8 , 61 ,411 , 3213 , 21637 , 158065}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 6, (perft_list) { 8 , 61 ,483 , 3213 , 23599 , 157093}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 6, (perft_list) { 4 , 15 ,89 , 537 , 3309 , 21104}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 6, (perft_list) { 4 , 19 ,117 , 720 , 4661 , 32191}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 6, (perft_list) { 5 , 19 ,116 , 716 , 4786 , 30980}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 6, (perft_list) { 3 , 15 ,84 , 573 , 3013 , 22886}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4271 , 28662}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 6, (perft_list) { 5 , 19 ,117 , 720 , 5014 , 32167}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 6, (perft_list) { 4 , 19 ,117 , 712 , 4658 , 30749}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 6, (perft_list) { 5 , 15 ,102 , 569 , 4337 , 22579}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4271 , 28662}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 6, (perft_list) { 5 , 25 ,161 , 1035 , 7574 , 55338}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 6, (perft_list) { 5 , 25 ,161 , 1035 , 7574 , 55338}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 6, (perft_list) { 7 , 49 ,378 , 2902 , 24122 , 199002}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 6, (perft_list) { 5 , 22 ,139 , 877 , 6112 , 41874}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 6, (perft_list) { 4 , 16 ,101 , 637 , 4354 , 29679}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 6, (perft_list) { 5 , 25 ,161 , 1035 , 7574 , 55338}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 6, (perft_list) { 5 , 25 ,161 , 1035 , 7574 , 55338}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 6, (perft_list) { 7 , 49 ,378 , 2902 , 24122 , 199002}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 6, (perft_list) { 11 , 97 ,887 , 8048 , 90606 , 1030499}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 6, (perft_list) { 24 , 421 ,7421 , 124608 , 2193768 , 37665329}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 6, (perft_list) { 18 , 270 ,4699 , 79355 , 1533145 , 28859283}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 6, (perft_list) { 24 , 496 ,9483 , 182838 , 3605103 , 71179139}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 6, (perft_list) { 11 , 97 ,887 , 8048 , 90606 , 1030499}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 6, (perft_list) { 24 , 421 ,7421 , 124608 , 2193768 , 37665329}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 6, (perft_list) { 18 , 270 ,4699 , 79355 , 1533145 , 28859283}, divide, use_classic, times_only) ? success++ : fail ++;
        perft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 6, (perft_list) { 24 , 496 ,9483 , 182838 , 3605103 , 71179139}, divide, use_classic, times_only) ? success++ : fail ++;
    }

    *s = *s + success;
    *f = *f + fail;
    if (use_classic) {
        printf("Classic ");
    } else {
        printf("Bitboard ");
    }
    printf("perft_tests result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

bool test_a_pinned_piece_position_bb(const char *fen, bool for_defense, struct SquareList answers, int pos) {
    struct bitChessBoard *pbb;
    struct SquareList tests, realanswers;
    int color_of_attackers, color_of_blockers;
    uint_64 testmask;
    int i,j;
    char *alg;
    bool ret, foundit;

    int kingpos;

    ret = true;
    pbb = new_bitboard();
    if (!load_bitboard_from_fen(pbb, fen)) {
        printf("Position %d: Invalid FEN in test pinned piece position: %s\n", pos, fen);
        ret = false;
    }
    else {


        if (!pbb->bSide_to_move) {
            if (for_defense) {
                kingpos = pbb->wk_pos;
                color_of_attackers = BLACK;
                color_of_blockers = WHITE;
            } else {
                kingpos = pbb->bk_pos;
                color_of_attackers = WHITE;
                color_of_blockers = WHITE;
            }
        } else {
            if (for_defense) {
                kingpos = pbb->bk_pos;
                color_of_attackers = WHITE;
                color_of_blockers = BLACK;
            } else {
                kingpos = pbb->wk_pos;
                color_of_attackers = BLACK;
                color_of_blockers = BLACK;
            }
        }

        SQUARELIST_CLEAR(&tests);
        testmask = generate_bb_pinned_list(pbb, kingpos, color_of_blockers, color_of_attackers);
        while (testmask) {
            SQUARELIST_ADD(&tests, pop_lsb(&testmask));
        }

        SQUARELIST_CLEAR(&realanswers);
        // the answers are in the 12x10 format, but we are in bitboard (8x8) format.
        for (i=0; i<answers.size; i++) {
            alg = arraypos_to_algebraic(answers.squares[i]);
            SQUARELIST_ADD(&realanswers, algebraic_to_bitpos(alg));
            free(alg);
        }

        if (tests.size != realanswers.size) {
            printf("BB Position %d: Pinned pieces for %s expected:%d  received:%d\n", pos, fen, realanswers.size, tests.size);
            ret = false;
        } else {
            for (i = 0; i < tests.size; i++) {
                foundit = false;
                for (j = 0; j < realanswers.size; j++) {
                    if (realanswers.squares[j] == tests.squares[i]) {
                        foundit = true;
                        break;
                    }
                }
                if (!foundit) {
                    printf("BB Position %d: Pinned piece FEN: %s found %d, did not expect it.\n", pos, fen, tests.squares[i]);
                    ret = false;
                }
            }
            for (i = 0; i < realanswers.size; i++) {
                foundit = false;
                for (j = 0; j < tests.size; j++) {
                    if (tests.squares[j] == realanswers.squares[i]) {
                        foundit = true;
                        break;
                    }
                }
                if (!foundit) {
                    printf("BB Position %d: Pinned piece FEN: %s expected %d, did not find it.\n", pos, fen, realanswers.squares[i]);
                    ret = false;
                }
            }
        }
    }
    free(pbb);
    return(ret);
}

bool test_a_pinned_piece_position_classic(const char *fen, bool for_defense, struct SquareList answers, int pos)
{
    struct ChessBoard *pb;
    struct SquareList tests;
    int i, j;
    bool ret, foundit;
    uc kingpos;
    bool test_sliders = false, test_diags = false;
    uc which_queen, which_king, which_rook, which_bishop;

    ret = true;
    pb = new_board();
    if (!load_from_fen(pb, fen)) {
        printf("Position %d: Invalid FEN in test pinned piece position: %s\n", pos, fen);
        ret = false;
    } else {
        if (((for_defense) && (pb->attrs & W_TO_MOVE)) || ((!for_defense) && (!(pb->attrs & W_TO_MOVE)))) {
            which_king = WK;
            which_queen = BQ;
            which_rook = BR;
            which_bishop = BB;
        } else {
            which_king = BK;
            which_queen = WQ;
            which_rook = WR;
            which_bishop = WB;
        }


        kingpos = find_next_piece_location(pb, which_king, 0);
        if (find_next_piece_location(pb, which_queen,0)) {
            test_sliders = true;
            test_diags = true;
        } else {
            if (find_next_piece_location(pb, which_rook, 0)) {
                test_sliders = true;
            }
            if (find_next_piece_location(pb, which_bishop, 0)) {
                test_diags = true;
            }
        }

        generate_pinned_list(pb, &tests, for_defense, kingpos, test_sliders, test_diags);
        if (tests.size != answers.size) {
            printf("Position %d: Pinned pieces for %s expected:%d  received:%d\n", pos, fen, answers.size, tests.size);
            ret = false;
        } else {
            for (i = 0; i < tests.size; i++) {
                foundit = false;
                for (j = 0; j < answers.size; j++) {
                    if (answers.squares[j] == tests.squares[i]) {
                        foundit = true;
                        break;
                    }
                }
                if (!foundit) {
                    printf("Position %d: Pinned piece FEN: %s found %d, did not expect it.\n", pos, fen, tests.squares[i]);
                    ret = false;
                }
            }
            for (i = 0; i < answers.size; i++) {
                foundit = false;
                for (j = 0; j < tests.size; j++) {
                    if (tests.squares[j] == answers.squares[i]) {
                        foundit = true;
                        break;
                    }
                }
                if (!foundit) {
                    printf("Position %d: Pinned piece FEN: %s expected %d, did not find it.\n", pos, fen, answers.squares[i]);
                    ret = false;
                }
            }
        }
    }
    free(pb);
    return ret;
}

bool test_a_pinned_piece_position(const char *fen, bool for_defense, struct SquareList answers, int pos, bool classic)
{
    if(classic) {
        return test_a_pinned_piece_position_classic(fen, for_defense, answers, pos);
    } else {
        return test_a_pinned_piece_position_bb(fen, for_defense, answers, pos);
    }

}

int test_pinned_and_discovered_checks(int *s, int *f, bool classic)
{
    int success = 0, fail = 0, pos = 0;
    struct SquareList answers;

    SQUARELIST_CLEAR(&answers);
    test_a_pinned_piece_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    // discovered check tests
    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 57);
    test_a_pinned_piece_position("8/2p5/3p4/KP5r/1R4Pk/5p2/4P3/8 w - - 0 1", false, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    test_a_pinned_piece_position("r1b2r1k/ppppnppp/2n1p3/1B2Pq2/3P1P2/2P3P1/P1P2K1P/R1BQ1R2 w - - 4 3", false, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 62);
    test_a_pinned_piece_position("8/8/8/kP5R/r4p1K/8/8/8 w - - 0 1", false, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 87);
    SQUARELIST_ADD(&answers, 44);
    test_a_pinned_piece_position("7B/6N1/8/8/3k4/3B4/3Q4/K7 w - - 0 1", false, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 66);
    test_a_pinned_piece_position("8/2p5/1P1p4/K4k1r/1R3pP1/8/4P3/8 b - - 0 3", false, answers, ++pos, classic) ? success++ : fail++;

    // pinned piece tests
    SQUARELIST_CLEAR(&answers);
    test_a_pinned_piece_position("k7/p7/8/8/8/Q7/K7/8 w - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;
    test_a_pinned_piece_position("k7/n7/8/8/8/B7/K7/8 b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 81);
    test_a_pinned_piece_position("k7/p7/8/8/8/Q7/K7/8 b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;
    test_a_pinned_piece_position("k7/p7/8/8/8/R7/K7/8 b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;
    test_a_pinned_piece_position("k7/n7/8/8/8/R7/K7/8 b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 82);
    test_a_pinned_piece_position("k7/nb6/8/8/8/B7/K7/7Q b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 81);
    SQUARELIST_ADD(&answers, 82);
    test_a_pinned_piece_position("k7/nb6/8/8/8/R7/K7/7Q b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 41);
    test_a_pinned_piece_position("k7/nb6/r7/8/8/R7/K7/7Q w - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 62);
    test_a_pinned_piece_position("8/8/8/KP5r/1R3p1k/8/6P1/8 w - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 56);
    test_a_pinned_piece_position("8/8/8/KP5r/1R3p1k/8/6P1/8 b - - 0 1", true, answers, ++pos, classic) ? success++ : fail++;
    test_a_pinned_piece_position("r1b2r1k/ppppnppp/2n1p3/1B2Pq2/3P1P2/2P3P1/P1P2K1P/R1BQ1R2 w - - 4 3", true, answers, ++pos, classic) ? success++ : fail ++;

    SQUARELIST_CLEAR(&answers);
    SQUARELIST_ADD(&answers, 84);
    test_a_pinned_piece_position("r3k2r/p1ppqpb1/bn2pnp1/1B1PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 0 1", true, answers, ++pos, classic) ? success++ : fail++;

    *s = *s + success;
    *f = *f + fail;
    if (!classic) {
        printf ("Bitboard ");
    }
    printf("pinned and discovered checks result:  Success: %d,  Failure: %d\n", success, fail);
    return 0;
}

int bitboard_tests(int *s, int *f)
{
    int success = 0, fail = 0, i;
    char *r;
    struct bitChessBoard *pbb;



    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "k7/8/8/8/8/8/8/7K w - - 0 1"); {
        if (pbb->piece_boards[ALL_PIECES] & SQUARE_MASKS[A8]) {
            success++;
        } else {
            printf("No piece on A8\n");
            fail++;
        }
        if (pbb->piece_boards[BK] & SQUARE_MASKS[A8]) {
            success++;
        } else {
            printf("BK not on A8\n");
            fail++;
        }
        if (pbb->piece_boards[ALL_PIECES] & SQUARE_MASKS[H1]) {
            success++;
        } else {
            printf("No piece on H1\n");
            fail++;
        }
        if (pbb->piece_boards[WK] & SQUARE_MASKS[H1]) {
            success++;
        } else {
            printf("WK not on H1\n");
            fail++;
        }
    }
    free(pbb);


    pbb = new_bitboard();
    set_bitboard_startpos(pbb);
    r = convert_bitboard_to_fen(pbb);
    if (strcmp (r, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") == 0) {
        success++;
    } else {
        printf("Failed BB Test #1: %s\n\n", r);
        for (i = 0; i < 64; i++) {
            debug_contents_of_bitboard_square(pbb, i);
        }
        fail++;
    }
    free(r);
    free(pbb);

    printf("bitboard tests result:  Success: %d,  Failure: %d\n", success, fail);
    *s += success;
    *f += fail;
    return 0;
}

void bitfunc_tests(int *s, int *f)
{
    int success = 0, fail = 0;
    uint_64 x;
    int i;

    x = 1042ul; // (2^10 + 2^4 + 2^1)
    i = pop_lsb(&x);
    if (i == 1) {
        success++;
    } else {
        printf("First popped bit should have been 1, instead %d\n", i);
    }
    i = pop_lsb(&x);
    if (i == 4) {
        success++;
    } else{
        printf("Second popped bit should have been 4, instead %d\n", i);
    }
    i = pop_lsb(&x);
    if (i==10) {
        success++;
    } else {
        printf("Third popped bit should have been 10, instead %d\n", i);
    }
    if (x==0) {
        success++;
    } else {
        printf("X should have been 0, instead it is %lu\n",x);
    }

    printf("bit functions result:  Success: %d,  Failure: %d\n", success, fail);
    *s += success;
    *f += fail;
}

void bitboard_movegen_tests(int *s, int *f)
{
    int success = 0;
    int fail = 0;

    struct bitChessBoard *pbb;
    struct MoveList ml;


    printf("\n\n");

    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "8/8/8/4k3/3n1r2/4K3/8/2N5 w - - 0 1");
    printf("Moves for 8/8/8/4k3/3n1r2/4K3/8/2N5 w - - 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    movelist_sort_alpha(&ml, true);
    free(pbb);;


    printf("\n\n");

    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "8/7P/6k1/1pP5/2P2P1P/8/P7/K7 w - b6 0 1");
    printf("8/7P/6k1/1pP5/2P2P1P/8/P7/K7 w - b6 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);

    printf("\n\n");
    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "k7/7P/8/8/8/8/P7/K7 w - - 0 1");
    printf("k7/7P/8/8/8/8/P7/K7 w - - 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);

    printf("\n\n");
    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "k7/1n6/2P5/8/8/8/8/K7 w - - 0 1");
    printf("k7/1n6/2P5/8/8/8/8/K7 w - - 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);

    printf("\n\n");
    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "k7/1n6/2P5/5pP1/8/8/8/K7 w - f6 0 1");
    printf("moves for k7/1n6/2P5/5pP1/8/8/8/K7 w - f6 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);

    printf("\n\n");
    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "k7/p7/8/8/4Pp2/6p1/5Q2/7K b - e3 0 1");
    printf("moves for k7/p7/8/8/4Pp2/6p1/5Q2/7K b - e3 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);



    printf("\n\n");
    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    set_bitboard_startpos(pbb);
    printf("moves for startpos\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);

    MOVELIST_CLEAR(&ml);
    pbb = new_bitboard();
    load_bitboard_from_fen(pbb, "k7/8/6Q1/8/8/3r4/8/7K w - - 0 1");
    printf("Moves for k7/8/6Q1/8/8/3r4/8/7K w - - 0 1\n");

    generate_bb_move_list(pbb, &ml);
    print_bb_move_list(&ml);
    free(pbb);



    printf("bitboard movegen test result:  Success: %d,  Failure %d\n", success, fail);
    *s += success;
    *f += fail;
}


bool movelist_comparison(const char *fen)
{
    struct ChessBoard *pb;
    struct bitChessBoard *pbb;
    struct MoveList classic;
    struct MoveList bb;
    int i,j;
    bool ret = true;
    bool foundit;
    bool printedfen = false;
    char *movestrclassic, *movestrbitboard;

    pb = new_board();
    pbb = new_bitboard();
    load_from_fen(pb, fen);
    load_bitboard_from_fen(pbb, fen);

    /*
    movestrbitboard = pretty_print_bb_move(151071779);
    movestrclassic = pretty_print_move(151079744);
    printf("Applying BB: %s,  Classic %s\n", movestrbitboard, movestrclassic);
    free(movestrbitboard);
    free(movestrclassic);

    apply_bb_move(pbb, 151071779);
    apply_move(pb, 151079744);
*/

    generate_move_list(pb, &classic);
    generate_bb_move_list(pbb, &bb);
    movelist_sort_alpha(&classic, true);
    movelist_sort_alpha(&bb, false);


    int cmp;

    if (classic.size != bb.size) {
        printf("Movelist comparison %s failed - classic = %d moves, bb = %d moves.\n", fen, classic.size, bb.size);
        printedfen = true;
        ret = false;
    }
    for (i=0; i<classic.size; i++) {
        foundit = false;
        movestrclassic = pretty_print_move(classic.moves[i]);
        for (j=0; j<bb.size; j++) {
            movestrbitboard = pretty_print_bb_move(bb.moves[j]);
            cmp = strcmp(movestrclassic, movestrbitboard);
            free(movestrbitboard);
            if (cmp == 0) {
                if (GET_FLAGS(classic.moves[i]) != GET_FLAGS(bb.moves[j])) {
                    printf("Move found in both with different flags - %s has flags %d classic, and %d bitboard", movestrclassic, GET_FLAGS(classic.moves[i]), GET_FLAGS(bb.moves[j]));
                }
                foundit = true;
                break;
            }
        }
        if (!foundit) {
            if (!printedfen) {
                printf("Movelist comparison %s failed ", fen);
                printedfen = true;
            }
            ret = false;
            printf("Move %s in classic, not in bitboard.\n", movestrclassic);
        }
        free (movestrclassic);
    }
    for (i=0; i<bb.size; i++) {
        foundit = false;
        movestrbitboard = pretty_print_bb_move(bb.moves[i]);
        for (j=0; j<classic.size; j++) {
            movestrclassic = pretty_print_move(classic.moves[j]);
            cmp = strcmp(movestrclassic, movestrbitboard);
            free(movestrclassic);
            if (cmp == 0) {
                foundit = true;
                break;
            }
        }
        if (!foundit) {
            if (!printedfen) {
                printf("Movelist comparison %s failed ", fen);
                printedfen = true;
            }
            ret = false;
            printf("Move %s in bitboard, not in classic.\n", movestrbitboard);
        }
        free (movestrbitboard);
    }

    if (ret) {
        printf("Move comparison %s succeeded.\n", fen);
/*

        printf("Classic Move list: \n");
        for (i = 0; i<classic.size; i++) {
            movestrclassic = pretty_print_move(classic.moves[i]);
            printf("%s (%ld) - flags: %d\n", movestrclassic, classic.moves[i], GET_FLAGS(classic.moves[i]));
            free(movestrclassic);
        }
        printf("\n\n");
        printf("Bitboard Move list: \n");
        for (i = 0; i < bb.size; i++) {
            movestrbitboard = pretty_print_bb_move(bb.moves[i]);
            printf("%s (%ld) - flags: %d\n", movestrbitboard, bb.moves[i], GET_FLAGS(bb.moves[i]));
            free(movestrbitboard);
        }
*/
    }

    return ret;
}


void perf1()
{
    clock_t start, stop;
    double elapsed;
    int i;

    struct ChessBoard *pb;
    struct bitChessBoard *pbb;
    struct MoveList ml;
    const int numreps = 1000000;

    const_bitmask_init();
    pb = new_board();
    pbb = new_bitboard();
    set_start_position(pb);
    set_bitboard_startpos(pbb);

    printf("Opening:\n");

    start = clock();
    for (i=0; i<numreps; i++) {
        MOVELIST_CLEAR(&ml);
        generate_move_list(pb, &ml);
    }
    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("Old way elapsed; %f ms\n", elapsed);

    start = clock();
    for (i=0; i<numreps; i++) {
        MOVELIST_CLEAR(&ml);
        generate_bb_move_list(pbb, &ml);
    }
    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("New way elapsed; %f ms\n\n", elapsed);

    printf("Midgame: 1r1q1rk1/2p1bppp/p5b1/3pP3/Bn1Pn3/2N1BN1P/1P2QPP1/R2R2K1 w - - 0 1\n");
    load_bitboard_from_fen(pbb, "1r1q1rk1/2p1bppp/p5b1/3pP3/Bn1Pn3/2N1BN1P/1P2QPP1/R2R2K1 w - - 0 1");
    load_from_fen(pb,"1r1q1rk1/2p1bppp/p5b1/3pP3/Bn1Pn3/2N1BN1P/1P2QPP1/R2R2K1 w - - 0 1");
    start = clock();
    for (i=0; i<numreps; i++) {
        MOVELIST_CLEAR(&ml);
        generate_move_list(pb, &ml);
    }
    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("Old way elapsed; %f ms\n", elapsed);

    start = clock();
    for (i=0; i<numreps; i++) {
        MOVELIST_CLEAR(&ml);
        generate_bb_move_list(pbb, &ml);
    }
    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("New way elapsed; %f ms\n\n", elapsed);

    printf("Endgame: r2r4/pp3p2/4bkpp/8/7P/3B1P2/PP4P1/1K1R3R b - - 0 1\n");
    load_bitboard_from_fen(pbb, "r2r4/pp3p2/4bkpp/8/7P/3B1P2/PP4P1/1K1R3R b - - 0 1");
    load_from_fen(pb,"r2r4/pp3p2/4bkpp/8/7P/3B1P2/PP4P1/1K1R3R b - - 0 1");
    start = clock();
    for (i=0; i<numreps; i++) {
        MOVELIST_CLEAR(&ml);
        generate_move_list(pb, &ml);
    }
    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("Old way elapsed; %f ms\n", elapsed);

    start = clock();
    for (i=0; i<numreps; i++) {
        MOVELIST_CLEAR(&ml);
        generate_bb_move_list(pbb, &ml);
    }
    stop = clock();
    elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
    printf("New way elapsed; %f ms\n\n", elapsed);


    free(pbb);
    free(pb);


}


void kind_tests()
{
    uint_64 test;

    // do a test for the bishop on b5 going NE/SW
    printf("First test - a4, d7 occupied\n");
    test = SQUARE_MASKS[A4] | SQUARE_MASKS[D7];
    // multiply by b file and shift 58
    test = test * B_FILE;
    test = test >> 58;

    printf("%lx\n",test);  // result is 4, we don't care about the A4 and D7 maps to bit 4.

    test = SQUARE_MASKS[C6] | SQUARE_MASKS[D7] | SQUARE_MASKS[E8];
    test = test * B_FILE;
    test = test >> 58;

    printf("%lx  %ld\n",test, test);  // result is 14 which is "e"

    // now try bishop on D3

    test = SQUARE_MASKS[B1] | SQUARE_MASKS[F5] | SQUARE_MASKS[G6];
    test = test * B_FILE;
    test = test >> 58;
    printf("%lx  %ld\n",test, test);  // result is 14 which is "e"

    printf("\n\n");




}



int main() {

    int success = 0, fail = 0;
    char * t;


#ifndef DISABLE_HASH
    TT_init(0);
    TT_init_bitboard(0);
#endif

    const_bitmask_init();


//    perf1();
//    perft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 6, (perft_list) { 6 , 35 ,495 , 8349 , 166741 , 3370175}, false, false, false) ? success++ : fail ++;


    bitboard_tests(&success, &fail);
    bitfunc_tests(&success, &fail);
    //bitboard_movegen_tests(&success, &fail);
    test_pinned_and_discovered_checks(&success, &fail, false);
    // The movelist_comparisons all isolated bugs in bitboard move generation in the past, so "pin" the fixes by keeping in unit tests.

    movelist_comparison("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1") ? success++ : fail++;
    movelist_comparison("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R4RK1 b kq - 0 1") ? success++ : fail++;
    movelist_comparison("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R4K1R b kq - 0 1") ? success++ : fail++;
    movelist_comparison("rnQq1k1r/pp2bppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R b KQ - 1 8") ? success++ : fail++;
    movelist_comparison("8/8/3p4/1Pp4r/KR3p1k/8/4P1P1/8 w - c6 0 1") ? success++ : fail++;
    movelist_comparison("rnb2k1r/pp1Pbppp/2p5/q7/2B5/P7/1PP1NnPP/RNBQK2R w KQ - 1 8") ? success++ : fail++;
    movelist_comparison("r1b2rk1/2p2ppp/p7/1p6/3P3q/1BP3bP/PP3QP1/RNB1R1K1 w - - 1 0") ? success++ : fail++;
    movelist_comparison("8/2p5/3p4/KP5r/1R4Pk/5p2/4P3/8 w - - 0 1") ? success++ : fail++;
    movelist_comparison("n1n5/PPP5/2k5/8/8/8/4Kppp/5N1N w - - 0 1") ? success++ : fail++;
    movelist_comparison("rnB2k1r/pp2bppp/B1p5/8/3q4/8/PPP1NnPP/RNBQK2R b KQ - 1 8") ? success++ : fail++;
    movelist_comparison("rnB2k1r/pp2bppp/B1p5/8/3r4/8/PPP1NnPP/RNBQK2R b KQ - 1 8") ? success++ : fail++;
    movelist_comparison("rnB2k1r/pp2bppp/B1p5/8/3b4/8/PPP1NnPP/RNBQK2R b KQ - 1 8") ? success++ : fail++;
    movelist_comparison("8/2p5/3p4/KP5r/1R2Pp1k/8/6P1/8 b - e3 0 1") ? success++ : fail++;
    movelist_comparison("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R w KQkq - 0 1") ? success++ : fail++;
    movelist_comparison("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R w KQkq - 0 1") ? success++ : fail++;
    movelist_comparison("n1n5/PPPk4/8/8/8/8/4Kp1p/5b1N w - - 0 2") ? success++ : fail++;
    movelist_comparison("8/2p5/1P1p4/K4k1r/1R3pP1/8/4P3/8 b - - 0 3") ? success++ : fail++;
    movelist_comparison("8/8/8/3k4/4Pp2/8/8/4KR2 b - e3 0 1") ? success++ : fail++;
    movelist_comparison("8/8/8/3k4/r3Pp1K/8/8/8 b - e3 0 1") ? success++ : fail++;
    movelist_comparison("8/8/8/5k2/4Pp2/8/8/4KR2 b - e3 0 1") ? success++ : fail++;
    movelist_comparison("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1") ? success++ : fail++;
    perft_tests(true, &success, &fail, false, false, false);

    //init_check_tables();
/*
    move_tests(false, &success, &fail);
    list_tests(&success, &fail);
    fen_tests(&success, &fail);
    apply_move_tests(&success, &fail);
    check_tests(&success, &fail);
    macro_tests(&success, &fail);
    test_pinned_and_discovered_checks(&success, &fail, true);
    perft_tests(false, &success, &fail, false, true, false);
*/

#ifndef DISABLE_HASH
    TT_destroy();
    TT_destroy_bitboard();
#ifndef NDEBUG
    printf("\n\n Hash: Inserts %ld, probes %ld\n", DEBUG_TT_INSERTS, DEBUG_TT_PROBES);
#endif
#endif



    printf("\n\n\nTOTAL: Success:%d   Fail: %d\n\n", success, fail);

    //printable_move_generation_tests();

    return 0;
}
