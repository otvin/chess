#pragma once

#include <stdbool.h>
#include "chess_constants.h"


/* board */

typedef struct ChessBoard {
    square squares[120];
    square ep_target;
    unsigned int halfmove_clock;
    unsigned int fullmove_number;
    uc attrs;
    //MoveList move_history;

} ChessBoard;

/* Constants for bits for attributes */
#define W_CASTLE_QUEEN (uc)1
#define W_CASTLE_KING (uc)2
#define B_CASTLE_QUEEN (uc)4
#define B_CASTLE_KING (uc)8
#define W_TO_MOVE (uc)16
#define BOARD_IN_CHECK (uc)32

bool arraypos_is_on_board(square pos);
uc algebraic_to_arraypos(char alg[2]);
square charpiece_to_square(char piece);
char square_to_charpiece(square s);
void erase_board(struct ChessBoard *pb);
void set_start_position(struct ChessBoard *pb);
bool load_from_fen(struct ChessBoard *pb, const char *fen);
char *print_board(const struct ChessBoard *pb);
struct ChessBoard *new_board();
void apply_move(struct ChessBoard *pb, Move m);
square find_next_piece_location(const struct ChessBoard *pb, uc piece, uc index);
bool side_to_move_is_in_check(const struct ChessBoard *pb);