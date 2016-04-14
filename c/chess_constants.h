/* Constants */

typedef unsigned char uc;  /* tired of typing so much */
typedef unsigned char square;


/* CONSTANTS for pieces.  7th bit is color */
#define PAWN 1
#define KNIGHT 2
#define BISHOP 4
#define ROOK 8
#define QUEEN 16
#define KING 32
#define BLACK 64

#define WP 1
#define BP 65 /* 65 = BLACK | PAWN */
#define WN 2
#define BN 66
#define WB 4
#define BB 68
#define WR 8
#define BR 76
#define WQ 16
#define BQ 80
#define WK 32
#define BK 96

#define EMPTY 0
#define OFF_BOARD 128

/* CONSTANTS for the bit field for attributes of the board. */
#define W_CASTLE_QUEEN 1
#define W_CASTLE_KING 2
#define B_CASTLE_QUEEN 4
#define B_CASTLE_KING 8
#define W_TO_MOVE 16
#define BOARD_IN_CHECK 32