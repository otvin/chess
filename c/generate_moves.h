#pragma once

#include "chessmove.h"
#include "chessboard.h"

/* Below structure is paraphrased from GNU Chess */
#define MAX_MOVELIST_SIZE 256

typedef struct MoveList {
    int size;
    Move moves[MAX_MOVELIST_SIZE];
} MoveList;

#define MOVELIST_ADD(list,move) ((list)->moves[(list)->size++]=(move))
#define MOVELIST_CLEAR(list) ((list)->size=0)

#define OPPOSITE_COLORS(p1,p2) ((p1 ^ p2) & BLACK)
#define SAME_COLORS(p1,p2) (!OPPOSITE_COLORS(p1,p2))

void print_move_list(const struct MoveList *list);
int generate_move_list(const ChessBoard *pb, MoveList *ml);
