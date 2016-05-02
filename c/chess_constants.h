#pragma once

/* Constants */

typedef unsigned char uc;  /* tired of typing so much */


/* CONSTANTS for pieces.  4th bit is color */
#define PAWN (uc)1
#define KNIGHT (uc)2
#define BISHOP (uc)3
#define ROOK (uc)4
#define QUEEN (uc)5
#define KING (uc)6
#define BLACK (uc)8
#define WHITE (uc)0

#define BB_BLACK 1
#define BB_WHITE 0
#define GET_BBCOLOR(piece) (!!(piece & 8))
#define PIECE_BITS(piece) ((piece) & 7)


#define WP PAWN
#define BP (BLACK | PAWN)
#define WN KNIGHT
#define BN (BLACK | KNIGHT)
#define WB BISHOP
#define BB (BLACK | BISHOP)
#define WR ROOK
#define BR (BLACK | ROOK)
#define WQ QUEEN
#define BQ (BLACK | QUEEN)
#define WK KING
#define BK (BLACK | KING)

#define EMPTY 0
#define OFF_BOARD 128

/* CONSTANTS for the bit field for attributes of the board. */
#define W_CASTLE_QUEEN (uc)1
#define W_CASTLE_KING (uc)2
#define B_CASTLE_QUEEN (uc)4
#define B_CASTLE_KING (uc)8
#define W_TO_MOVE (uc)16
#define BOARD_IN_CHECK (uc)32




#ifdef __GNUC__
#define if_unlikely(x) if(__builtin_expect(!!(x),0))
#else
#define if_unlikely(x) if(x)
#endif