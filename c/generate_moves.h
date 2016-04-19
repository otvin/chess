#pragma once

#include <stdbool.h>
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
    uc squares[MAX_SQUARELIST_SIZE];
} SquareList;

#define MOVELIST_ADD(list,move) ((list)->moves[(list)->size++]=(move))
#define MOVELIST_CLEAR(list) ((list)->size=0)

#define SQUARELIST_ADD(list,square) ((list)->squares[(list)->size++]=(square))
#define SQUARELIST_CLEAR(list) ((list)->size=0)

#define OPPOSITE_COLORS(p1,p2) ((p1 ^ p2) & BLACK)
#define SAME_COLORS(p1,p2) (!OPPOSITE_COLORS(p1,p2))



void print_move_list(const struct MoveList *list);
void movelist_remove(struct MoveList *ml, int position);
void squarelist_remove(struct SquareList *sl, int position);
bool square_in_list(const struct SquareList *sl, uc square);
int generate_move_list(const struct ChessBoard *pb, MoveList *ml);
void generate_pinned_list(const struct ChessBoard *pb, SquareList *sl, bool for_defense, uc kingpos, bool include_sliders, bool include_diags);
