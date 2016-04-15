#pragma once


#include <stdbool.h>


/* CONSTANTS for moves */

typedef unsigned long Move;

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
#define CAPTURE_DIFFERENTIAL 281470681743360ul
#define PROMOTED_TO 71776119061217280ul
#define MOVE_FLAGS 18374686479671623680ul


#define START_SHIFT 0
#define END_SHIFT 8
#define PIECE_MOVING_SHIFT 16
#define PIECE_CAPTURED_SHIFT 24
#define CAPTURE_DIFFERENTIAL_SHIFT 32
#define CAPTURE_DIFFERENTIAL_OFFSET (unsigned short)32767
#define PROMOTED_TO_SHIFT 48
#define MOVE_FLAGS_SHIFT 56


/* The move flags */
#define MOVE_CASTLE (uc)1
#define MOVE_EN_PASSANT (uc)2
#define MOVE_CHECK (uc)4
#define MOVE_DOUBLE_PAWN (uc)8
#define NULL_MOVE (Move)0

typedef struct MoveListNode {
    Move m;
    struct MoveListNode *next;
} MoveListNode;

typedef struct MoveList {
    struct MoveListNode *first;
    struct MoveListNode *last;
} MoveList;


Move create_move(square start, square end, uc piece_moving, uc piece_captured, short capture_differential, uc promoted_to, uc move_flags);
char * pretty_print_move(Move move);
struct MoveList *new_empty_move_list();
void add_move_to_list(struct MoveList *pList, Move move);
void delete_moves_in_list(struct MoveList *pList);
void print_move_list(struct MoveList list);
bool parse_move(Move move, square *pStart, square *pEnd, uc *pPiece_moving, uc *pPiece_captured, short *pCapture_differential, uc *pPromoted_to, uc *pMove_flags);