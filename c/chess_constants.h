#pragma once

/* Constants */

typedef unsigned char uc;  /* tired of typing so much */
typedef unsigned char square;


/* CONSTANTS for pieces.  7th bit is color */
#define PAWN (uc)1
#define KNIGHT (uc)2
#define BISHOP (uc)4
#define ROOK (uc)8
#define QUEEN (uc)16
#define KING (uc)32
#define BLACK (uc)64

#define WP PAWN
#define BP BLACK | PAWN
#define WN KNIGHT
#define BN BLACK | KNIGHT
#define WB BISHOP
#define BB BLACK | BISHOP
#define WR ROOK
#define BR BLACK | ROOK
#define WQ QUEEN
#define BQ BLACK | QUEEN
#define WK KING
#define BK BLACK | KING

#define EMPTY 0
#define OFF_BOARD 128

/* CONSTANTS for the bit field for attributes of the board. */
#define W_CASTLE_QUEEN (uc)1
#define W_CASTLE_KING (uc)2
#define B_CASTLE_QUEEN (uc)4
#define B_CASTLE_KING (uc)8
#define W_TO_MOVE (uc)16
#define BOARD_IN_CHECK (uc)32