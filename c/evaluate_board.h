#pragma once

#include "chess_constants.h"

#define PAWN_VALUE 100
#define KNIGHT_VALUE 290
#define BISHOP_VALUE 300
#define ROOK_VALUE 500
#define QUEEN_VALUE 900
#define KING_VALUE 20000

int piece_value(uc piece);