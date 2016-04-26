#pragma once

#include <assert.h>

#include "chess_constants.h"
#include "chessmove.h"
#include "generate_moves.h"

#ifndef NDEBUG
#define VALIDATE_BITBOARD_EACH_STEP 1
#endif

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


// NOTE: Below constants generated by bitboard_constant_generation.c and then pasted here for faster execution.

// Masks to find specific squares
// Example: D1 is enum 3, so square_masks[3] would equal 2^3 - a bit mask that had one bit set for the corresponding square
// not_masks[3] would be the inverse - every bit set except for 2^3, used to clear a bit.





// Masks to find specific squares
// Example: D1 is enum 3, so square_masks[3] would equal 2^3 - a bit mask that had one bit set for the corresponding square
// not_masks[3] would be the inverse - every bit set except for 2^3, used to clear a bit.
extern const uint_64 SQUARE_MASKS[64];
extern const uint_64 NOT_MASKS[64];

// Masks to find the edges of the board, or squares not on a given edge.
extern const uint_64 A_FILE;
extern const uint_64 H_FILE;
extern const uint_64 RANK_1;
extern const uint_64 RANK_8;
extern const uint_64 NOT_A_FILE;
extern const uint_64 NOT_H_FILE;
extern const uint_64 NOT_RANK_1;
extern const uint_64 NOT_RANK_8;

// Masks for 2 squares in from an edge, used to compute knight moves
extern const uint_64 B_FILE;
extern const uint_64 G_FILE;
extern const uint_64 RANK_2;
extern const uint_64 RANK_3;
extern const uint_64 RANK_6;
extern const uint_64 RANK_7;
extern const uint_64 NOT_B_FILE;
extern const uint_64 NOT_G_FILE;
extern const uint_64 NOT_RANK_2;
extern const uint_64 NOT_RANK_7;

// Masks for use in move generation
extern const uint_64 KNIGHT_MOVES[64];
extern const uint_64 KING_MOVES[64];
extern const uint_64 SLIDER_MOVES[64];
extern const uint_64 DIAGONAL_MOVES[64];

extern const uint_64 WHITE_PAWN_ATTACKSTO[64];
extern const uint_64 BLACK_PAWN_ATTACKSTO[64];

// Given a From and a To location, identify all the squares between them, if the squares share a rank, file, diagonal, or anti-diagonal
extern uint_64 SQUARES_BETWEEN[64][64];



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
    int castling;
    int side_to_move;  // 0 = WHITE, 8 = BLACK;
    bool in_check;
    int halfmoves_completed;
    Move move_history[MAX_MOVE_HISTORY];
    int wk_pos;
    int bk_pos;
#ifndef DISABLE_HASH
    unsigned long hash;
#endif
} bitChessBoard;


// Idea taking from FRC-Perft - for each start square, we have the masks that we would apply to the castling
// mask - makes it much easier to just do one operation as oppose to testing to see if King or Rook moved.
// Concept - pbb->castling &= castle_move_mask[start].
extern const int castle_move_mask[64];
// Similar - mask for the squares that need to be empty in order for castle to be valid, saves multiple adds/lookups at movegen time.
// it is an 8 by 2 array, So we can use color_moving as the index (choices are 0 and 8).
extern uint_64 castle_empty_square_mask[9][2];





#ifndef new_poplsb

#define GET_LSB(i) __builtin_ctzll(i)

static inline int pop_lsb(uint_64 *i)
{
    // returns 0-63 which would be the position of the first 1.  Passing in 0 would return 64 from ctzll, and that would cause
    // callers to barf.  Originally I had an if test there, but for performance reasons I don't want that branch in this code.
    assert(*i > 0);

    // TODO - see if there is a way to optimize this as it will be done a ton.
    int lsb;
    lsb = GET_LSB(*i);
    *i &= NOT_MASKS[lsb];
    return lsb;
}
// just like pop_lsb but doesn't modify the value passed in.

#endif

#ifdef new_poplsb
// Code stolen from FRC-Perft
static inline uint_64 bsfq(uint_64 mask) {
    uint_64 result;
    __asm__ (
    "bsfq %[mask], %[result]"
    :[result] "=r" (result)
    :[mask  ] "mr" (mask  )
    );
    return result;
}

//inline assembly version for 64-bit systems
static inline int pop_lsb(uint_64 *i) {
    int result = bsfq(*i);
    //clear least significant bit
    *i &= (*i)-1;
    return result;
}
#endif

bool const_bitmask_init();

struct bitChessBoard *new_bitboard();
void debug_contents_of_bitboard_square(const struct bitChessBoard *pbb, int square);

int algebraic_to_bitpos(const char alg[2]);
bool erase_bitboard(struct bitChessBoard *pbb);
bool set_bitboard_startpos(struct bitChessBoard *pbb);
bool load_bitboard_from_fen(struct bitChessBoard *pbb, const char *fen);
char *convert_bitboard_to_fen(const struct bitChessBoard *pbb);
uint_64 generate_bb_pinned_list(const struct bitChessBoard *pbb, int square, int color_of_blockers, int color_of_attackers);

int generate_bb_move_list(const struct bitChessBoard *pbb, MoveList *ml);
bool apply_bb_move(struct bitChessBoard *pbb, Move m);

bool validate_board_sanity(struct bitChessBoard *pbb);
