#include <stdio.h>
#include <string.h>
#include "chess_constants.h"
#include "chessmove.h"
#include "chessboard.h"

int move_tests()
{
    Move m;
    char *movestr;
    int success = 0;
    int fail = 0;

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