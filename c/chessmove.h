#pragma once

#include <stdbool.h>
#include "chess_constants.h"

/* CONSTANTS for moves */

#ifdef __GNUC__
typedef unsigned long Move;
#else
typedef unsigned long long Move;
#endif



/* Moves are 8 bytes.  Structure:
    Smallest 8 bytes = Start square
    Next 8 bytes = End square
    Next 8 bytes = piece moving
    Next 8 bytes = piece captured - or zero if no pieces captured
    Next 16 bytes = capture differential - the value of the piece being captured minus value of the piece moving, used to sort moves
    Next 8 bytes = pawn promotion piece, or zero if the move is not a promotion
    Final 8 bytes - flags for attributes of the move.
*/
#define START 255ul
#define END 65280ul
#define PIECE_MOVING 16711680ul
#define PIECE_CAPTURED 4278190080ul
#define PROMOTED_TO 71776119061217280ul
#define MOVE_FLAGS 18374686479671623680ul


#define START_SHIFT 0
#define END_SHIFT 8
#define PIECE_MOVING_SHIFT 16
#define PIECE_CAPTURED_SHIFT 24
#define PROMOTED_TO_SHIFT 48
#define MOVE_FLAGS_SHIFT 56


/* The move flags */
#define MOVE_CASTLE (uc)1
#define MOVE_EN_PASSANT (uc)2
#define MOVE_CHECK (uc)4
#define MOVE_DOUBLE_PAWN (uc)8
#define NULL_MOVE (Move)0

#define GET_START(move) ((uc)(move & START))
#define GET_END(move) ((uc)((move & END) >> END_SHIFT))
#define GET_PIECE_MOVING(move) ((uc)((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT))
#define GET_PIECE_CAPTURED(move) ((uc)((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT))
#define GET_PROMOTED_TO(move) ((uc)((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT))
#define GET_FLAGS(move) ((uc)((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT))

#define CREATE_MOVE(start, end, piece_moving, piece_captured, promoted_to, move_flags) ((start) | (((Move)(end)) << END_SHIFT) | (((Move)(piece_moving)) << PIECE_MOVING_SHIFT) | (((Move)(piece_captured)) << PIECE_CAPTURED_SHIFT) | (((Move)(promoted_to)) << PROMOTED_TO_SHIFT) | (((Move)(move_flags)) << MOVE_FLAGS_SHIFT))

// prelude to potentially using a 32-bit int for a BB move, no longer storing piece_moving.
#define CREATE_BB_MOVE(start, end, piece_captured, promoted_to, move_flags)  ((start) | (((Move)(end)) << END_SHIFT) | (((Move)(piece_captured)) << PIECE_CAPTURED_SHIFT) | (((Move)(promoted_to)) << PROMOTED_TO_SHIFT) | (((Move)(move_flags)) << MOVE_FLAGS_SHIFT))


Move create_move(uc start, uc end, uc piece_moving, uc piece_captured,  uc promoted_to, uc move_flags);
char * pretty_print_move(Move move);
char * pretty_print_bb_move(Move move);
bool parse_move(Move move, uc *pStart, uc *pEnd, uc *pPiece_moving, uc *pPiece_captured, uc *pPromoted_to, uc *pMove_flags);