#include <stdlib.h>
#include <stdio.h>

#include "chessmove.h"
#include "generate_moves.h"
#include "evaluate_board.h"

void print_move_list(const struct MoveList *list)
{
    char *movestr;
    int i;

    for (i = 0; i < list->size; i++) {
        movestr = pretty_print_move(list->moves[i]);
        printf("%s\n", movestr);
        free(movestr);
    }
}

void generate_pawn_moves(const ChessBoard *pb, MoveList *ml, square s)
{

}

void generate_knight_moves(const ChessBoard *pb, MoveList *ml, square s)
{
    int delta[8] = {-21, -19, -12, -8, 21, 19, 12, 8};
    uc piece, dest;
    square curpos;
    int i;

    piece = pb->squares[s];
    for (i = 0; i < 8; i++) {
        curpos = s + delta[i];
        dest = pb->squares[curpos];
        if (dest == EMPTY) {
            MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, 0));
        } else {
            if (dest != OFF_BOARD && OPPOSITE_COLORS(dest, piece)) {
                MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), 0, 0));
            }
        }
    }
}

void generate_king_moves(const ChessBoard *pb, MoveList *ml, square s)
{

}

void generate_directional_moves(const ChessBoard *pb, MoveList *ml, char velocity, square s)
{
    square curpos;
    uc piece, dest;

    piece = pb->squares[s];
    curpos = s + velocity;
    dest = pb->squares[curpos];
    while (dest == EMPTY) {
        MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, 0));
        curpos = curpos + velocity;
        dest = pb->squares[curpos];
    }
    if (dest != OFF_BOARD && OPPOSITE_COLORS(dest,piece)) {
        MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), 0, 0));
    }

}

void generate_diagonal_moves(const ChessBoard *pb, MoveList *ml, square s)
{
    generate_directional_moves(pb, ml, -9, s);
    generate_directional_moves(pb, ml, 9, s);
    generate_directional_moves(pb, ml, 11, s);
    generate_directional_moves(pb, ml, -11, s);
}

void generate_slide_moves(const ChessBoard *pb, MoveList *ml, square s)
{
    generate_directional_moves(pb, ml, -1, s);
    generate_directional_moves(pb, ml, 1, s);
    generate_directional_moves(pb, ml, 10, s);
    generate_directional_moves(pb, ml, -10, s);
}

int generate_move_list(const ChessBoard *pb, MoveList *ml)
{
    uc piece;
    square i;
    uc color_moving;

    color_moving = (pb->attrs & W_TO_MOVE) ? WHITE : BLACK;

    for (i=21;i<=98;i++) {
        piece = pb->squares[i];
        if (piece != EMPTY && piece != OFF_BOARD) {
            if (SAME_COLORS(piece,color_moving)) {
                if (piece & PAWN) {
                    generate_pawn_moves(pb, ml, i);
                } else if (piece & KNIGHT) {
                    generate_knight_moves(pb, ml, i);
                } else if (piece & KING) {
                    generate_king_moves(pb, ml, i);
                } else if (piece & BISHOP) {
                    generate_diagonal_moves(pb, ml, i);
                } else if (piece & ROOK) {
                    generate_slide_moves(pb, ml, i);
                } else if (piece & QUEEN) {
                    generate_diagonal_moves(pb, ml, i);
                    generate_slide_moves(pb, ml, i);
                }
            }
        }
    }
}