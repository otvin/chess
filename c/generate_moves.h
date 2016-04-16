#pragma once

#include "chessmove.h"
#include "chessboard.h"

/* Below structure is paraphrased from GNU Chess */
#define MAX_MOVELIST_SIZE 256
#define MAX_SQUARELIST_SIZE 64

typedef struct MoveList {
    int size;
    Move moves[MAX_MOVELIST_SIZE];
} MoveList;

typedef struct SquareList {
    int size;
    square squares[MAX_SQUARELIST_SIZE];
} SquareList;

#define MOVELIST_ADD(list,move) ((list)->moves[(list)->size++]=(move))
#define MOVELIST_CLEAR(list) ((list)->size=0)

#define SQUARELIST_ADD(list,square) ((list)->squares[(list)->size++]=(square))
#define SQUARELIST_CLEAR(list) ((list)->size=0)

#define OPPOSITE_COLORS(p1,p2) ((p1 ^ p2) & BLACK)
#define SAME_COLORS(p1,p2) (!OPPOSITE_COLORS(p1,p2))

void print_move_list(const struct MoveList *list);
void movelist_remove(MoveList *ml, int position);
void squarelist_remove(SquareList *sl, int position);
int generate_move_list(const ChessBoard *pb, MoveList *ml);
