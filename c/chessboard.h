#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "chess_constants.h"

/* board */

typedef struct ChessBoard {
    square squares[120];
    square ep_target;
    /*
    unsigned int halfmove_clock;
    unsigned int fullmove_number;
    Move *move_history;
    */
} ChessBoard;

bool arraypos_is_on_board(square pos);
void erase_board(struct ChessBoard *pb);
void set_start_position(struct ChessBoard *pb);
int print_board(struct ChessBoard *pb);
struct ChessBoard *new_board();