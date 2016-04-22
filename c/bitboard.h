#pragma once
#include "chess_constants.h"
#include "chessmove.h"
#include "generate_moves.h"

// Bitboard-based definition

typedef unsigned long uint_64;

// The order of the enum shows how the bits in the bitboard map to squares.  Least significant bit would be
// square A1, most significant would be H8.
typedef enum boardlayout {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
} boardlayout;


// Masks to find specific squares
// Example: D1 is enum 3, so square_masks[3] would equal 2^3 - a bit mask that had one bit set for the corresponding square
// not_masks[3] would be the inverse - every bit set except for 2^3, used to clear a bit.
extern uint_64 SQUARE_MASKS[64];
extern uint_64 NOT_MASKS[64];

// Masks to find the edges of the board, or squares not on a given edge.
extern uint_64 A_FILE;
extern uint_64 H_FILE;
extern uint_64 RANK_1;
extern uint_64 RANK_8;
extern uint_64 NOT_A_FILE;
extern uint_64 NOT_H_FILE;
extern uint_64 NOT_RANK_1;
extern uint_64 NOT_RANK_8;

// Masks for 2 squares in from an edge, used to compute knight moves
extern uint_64 B_FILE;
extern uint_64 G_FILE;
extern uint_64 RANK_2;
extern uint_64 RANK_7;
extern uint_64 NOT_B_FILE;
extern uint_64 NOT_G_FILE;
extern uint_64 NOT_RANK_2;
extern uint_64 NOT_RANK_7;

// Masks for use in move generation
extern uint_64 KNIGHT_MOVES[64];
extern uint_64 KING_MOVES[64];
extern uint_64 SLIDER_MOVES[64];
extern uint_64 DIAGONAL_MOVES[64];

// using constants from chess_constants.h - board 0 = WHITE (all White Pieces)
// 1-6 would be white pieces.  8 = BLACK (all Black pieces), then 9-14 would be the
// black pieces.  7 is unused, so we will use that slot for the "All pieces" bitboard, and
// then 15 for the inverse of all pieces which is the empty squares.
#define ALL_PIECES 7
#define EMPTY_SQUARES 15

// defined in chessboard.h - don't want to include that from here, for now.
#ifndef MAX_MOVE_HISTORY
#define MAX_MOVE_HISTORY 256
#endif

typedef struct bitChessBoard {
    uint_64 piece_boards[16];
    int ep_target;
    int halfmove_clock;
    int fullmove_number;
    int attrs;
    int halfmoves_completed;
    Move move_history[MAX_MOVE_HISTORY];
    int wk_pos;
    int bk_pos;
    unsigned long hash;
} bitChessBoard;


// https://chessprogramming.wikispaces.com/General+Setwise+Operations talks about how to pop LSB if there is no instruction
int pop_lsb(uint_64 *i);  // returns 1-64 the least significant bit and zeros that bit out of the integer passed in.
// look at ffsl() in string.h


bool const_bitmask_init();
void const_bitmask_verify();  // debugging only

struct bitChessBoard *new_bitboard();
void debug_contents_of_bitboard_square(const struct bitChessBoard *pbb, int square);

int algebraic_to_bitpos(const char alg[2]);
bool erase_bitboard(struct bitChessBoard *pbb);
bool set_bitboard_startpos(struct bitChessBoard *pbb);
bool load_bitboard_from_fen(struct bitChessBoard *pbb, const char *fen);
char *convert_bitboard_to_fen(const struct bitChessBoard *pbb);
bool bitboard_side_to_move_is_in_check(const struct bitChessBoard *pbb);

int generate_bb_move_list(const struct bitChessBoard *pbb, MoveList *ml);