# cython: language_level=3

import argparse

import signal
import sys
from datetime import datetime
from cpython cimport datetime

from copy import deepcopy
import random

from operator import xor
import colorama

from check_tables cimport WHITE_CHECK_TABLE, BLACK_CHECK_TABLE

# global variables

cdef:
    datetime.datetime START_TIME = datetime.now()
    bint DEBUG = False
    bint XBOARD = False
    bint POST = False
    long long NODES = 0
    list KILLER_MOVELIST = []



# constants for transposition table access
cdef:
    int CACHE_SCORE_EXACT = 0
    int CACHE_SCORE_HIGH = 1
    int CACHE_SCORE_LOW = 2

DEBUGFILE = None


# CONSTANTS for pieces.  7th bit is color
cdef:
    int PAWN = 1
    int KNIGHT = 2
    int BISHOP = 4
    int ROOK = 8
    int QUEEN = 16
    int KING = 32
    int BLACK = 64
    int WHITE = 0

    int WP = PAWN
    int BP = BLACK | PAWN
    int WN = KNIGHT
    int BN = BLACK | KNIGHT
    int WB = BISHOP
    int BB = BLACK | BISHOP
    int WR = ROOK
    int BR = BLACK | ROOK
    int WQ = QUEEN
    int BQ = BLACK | QUEEN
    int WK = KING
    int BK = BLACK | KING
    int EMPTY = 0
    int OFF_BOARD = 128

    # CONSTANTS for the bit field for attributes of the board.
cdef:
    unsigned int W_CASTLE_QUEEN = 1
    unsigned int W_CASTLE_KING = 2
    unsigned int B_CASTLE_QUEEN = 4
    unsigned int B_CASTLE_KING = 8
    unsigned int W_TO_MOVE = 16
    unsigned int BOARD_IN_CHECK = 32

# Originally a ChessMove was a class.  However, the overhead with creating objects is much higher than the
# overhead of creating lists, so I changed data structures.  List is of the following format:
# (FROM, TO, PIECE MOVING, PIECE CAPTURED, CAPTURE DIFFERENTIAL, PROMOTED TO, FLAGS)
# FROM and TO: Integer from 21..98
# PIECE MOVING, PIECE CAPTURED, PROMOTED TO: one of the constants, e.g. WP for White Pawn, BQ for Black Queen
#   EMPTY (0) for Captured or Promoted pieces means it was not capture / promotion
# CAPTURE DIFFERENTIAL: The value of the piece being captured, less the piece capturing.  Used to sort moves.
# FLAGS a bit field
#   1 = Castle, 2 = En Passant Capture, 4 = Move results in Check, 8 = 2-square pawn move
# Tuple is likely faster, but we set a couple flags after the move is initially created, and tuples aren't mutable.

# CYTHON ONLY - move is a long long - 64 bits.  Capture Differential is 16 bits, everything else is 8.
ctypedef long long Move
cdef long long temp_mask = 255
cdef:
    long long START = 0
    long long END = 0
    long long PIECE_MOVING = 0
    long long PIECE_CAPTURED = 0
    long long CAPTURE_DIFFERENTIAL = 0
    long long PROMOTED_TO = 0
    long long MOVE_FLAGS = 0

cdef:
    int START_SHIFT = 0
    int END_SHIFT = 8
    int PIECE_MOVING_SHIFT = 16
    int PIECE_CAPTURED_SHIFT = 24
    int CAPTURE_DIFFERENTIAL_SHIFT = 32
    int PROMOTED_TO_SHIFT = 48
    int MOVE_FLAGS_SHIFT = 56

START |= temp_mask
END |= (temp_mask << END_SHIFT)
PIECE_MOVING |= (temp_mask << PIECE_MOVING_SHIFT)
PIECE_CAPTURED |= (temp_mask << PIECE_CAPTURED_SHIFT)
CAPTURE_DIFFERENTIAL |= (temp_mask << CAPTURE_DIFFERENTIAL_SHIFT)
CAPTURE_DIFFERENTIAL |= (temp_mask << (CAPTURE_DIFFERENTIAL_SHIFT + 8))
PROMOTED_TO |= (temp_mask << PROMOTED_TO_SHIFT)
MOVE_FLAGS |= (temp_mask << MOVE_FLAGS_SHIFT)

cdef int CAPTURE_DIFFERENTIAL_OFFSET = 32767

cdef Move create_move (long long start, long long end, long long piece_moving, long long piece_captured,
                            long long capture_differential,
                            long long promoted_to, long long move_flags):

    cdef Move ret = 0

    ret |= start
    ret |= (end << END_SHIFT)
    ret |= (piece_moving << PIECE_MOVING_SHIFT)
    ret |= (piece_captured << PIECE_CAPTURED_SHIFT)
    # negative capture differentials were causing overflow, so adding 32767
    ret |= ((CAPTURE_DIFFERENTIAL_OFFSET + capture_differential) << CAPTURE_DIFFERENTIAL_SHIFT)
    ret |= (promoted_to << PROMOTED_TO_SHIFT)
    ret |= (move_flags << MOVE_FLAGS_SHIFT)

    return ret

# Bits in the Flags
cdef:
    int MOVE_CASTLE = 1
    int MOVE_EN_PASSANT = 2
    int MOVE_CHECK = 4
    int MOVE_DOUBLE_PAWN = 8

# Special move used to make comparisons fast
cdef Move NULL_MOVE = 0

# moving the hash positions from a dict/list combination to an array.  This will be modestly better performing
# in python but much better in Cython since it will use an actual C array.
cdef unsigned long long HISTORICAL_HASH_ARRAY[150]
cdef int HISTORICAL_HASH_POSITION = -1  # we increment before we add to the array.
cdef int HISTORICAL_HASH_MIN_POSITION = 0  # the position where the halfmove clock was last set to 0.

cdef init_historical_hash_array():
    global HISTORICAL_HASH_ARRAY, HISTORICAL_HASH_POSITION, HISTORICAL_HASH_MIN_POSITION
    cdef int i
    for i in range(150):
        HISTORICAL_HASH_ARRAY[i] = 0
    HISTORICAL_HASH_POSITION = -1
    HISTORICAL_HASH_MIN_POSITION = 0

init_historical_hash_array()





# Helper functions

cdef int algebraic_to_arraypos(algebraicpos):
    """
    :param algebraicpos: from "a1".."h8"
    :return: an integer from 0..119 = space in the array corresponding to the square represented by algebraicpos
    """
    cdef int retval

    assert(len(algebraicpos) == 2)
    assert(algebraicpos[1].isnumeric())

    file = algebraicpos[0].lower()
    rank = int(algebraicpos[1])

    assert('a' <= file <= 'h')
    assert(1 <= rank <= 8)

    retval = (10 * (rank+1)) + 1
    retval += (ord(file)-97)  # 97 is the ascii for "a"
    return retval


cdef str arraypos_to_algebraic(int arraypos):
    """
    :param arraypos: an integer from 0..119 which is the space in the array corresponding to the square
                    however - only from 21..98 are actually on the board, excluding numbers ending in 0 or 9
    :return: an algebraic description of the square from "a1".."h8"
    """
    assert (21 <= arraypos <= 98)
    assert (1 <= (arraypos % 10) <= 9)

    file = chr(97 + (arraypos % 10) - 1)
    rank = (arraypos // 10) - 1
    return file + str(rank)


cdef bint arraypos_is_on_board(int arraypos):

    # layout of the board - count this way from 0..119

    # 110 111 112 113 114 115 116 117 118 119
    # 100 101 102 103 104 105 106 107 108 109
    # ...
    # 10 11 12 13 14 15 16 17 18 19
    # 00 01 02 03 04 05 06 07 08 09

    # all squares with 0 or 9 in the 1's space are off the board
    # all squares with tens digit 0 or 1 (which includes 100 and 110) are off the board

    cdef bint retval = True

    mod_ten = arraypos % 10
    if mod_ten == 0 or mod_ten == 9:
        retval = False
    else:
        tens_digit = (arraypos // 10) % 10
        if tens_digit == 0 or tens_digit == 1:
            retval = False
    return retval

# initialization of piece-square-tables
# layout of the board - count this way from 0..119

# 110 111 112 113 114 115 116 117 118 119
# 100 101 102 103 104 105 106 107 108 109
# ...
# 10 11 12 13 14 15 16 17 18 19
# 00 01 02 03 04 05 06 07 08 09

# start position is a list based on this (looks mirrored, keep in mind)
#   'xxxxxxxxxx'
#   'xxxxxxxxxx'
#   'xRNBQKBNRx'
#   'xPPPPPPPPx'
#   'x        x'
#   'x        x'
#   'x        x'
#   'x        x'
#   'xppppppppx'
#   'xrnbqkbnrx'
#   'xxxxxxxxxx'
#   'xxxxxxxxxx'

cdef int ob = -32767  # short for "off board"

# for KP vs K endgame, for each pawn position, identify the key squares, where if the King occupies them, the
# pawn will promote, regardless who moves next, unless enemy king is adjacent to pawn and it is enemy king's move
# See: https://en.wikipedi.org/wiki/King_and_pawn_versus_king_endgame
cdef dict white_kpk_key_squares = {
    31: [82,92], 41: [82,92], 51: [82,92], 61: [82,92], 71: [82,92], 81: [82,92],
    38: [87,97], 48: [87,97], 58: [87,97], 68: [87,97], 78: [87,97], 88: [87,97],

    32: [51,52,53], 33: [52,53,54], 34: [53,54,55], 35: [54,55,56], 36: [55,56,57], 37: [56,67,58],
    42: [61,62,63], 43: [62,63,64], 44: [63,64,65], 45: [64,65,66], 46: [65,66,67], 47: [66,67,68],
    52: [71,72,73], 53: [72,73,74], 54: [73,74,75], 55: [74,75,76], 56: [75,76,77], 57: [76,77,78],

    62: [71,72,73,81,82,83], 63: [72,73,74,82,83,84], 64: [73,74,75,83,84,85],
    65: [74,75,76,84,85,86], 66: [75,76,77,85,86,87], 67: [76,77,78,86,87,88],

    72: [81,82,83,91,92,93], 73: [82,83,84,92,93,94], 74: [83,84,85,93,94,95],
    75: [84,85,86,94,95,96], 76: [85,86,87,95,96,97], 77: [86,87,88,96,97,98],

    82: [81,83,91,92,93], 83: [82,84,92,93,94], 84: [83,85,93,94,95],
    85: [84,86,94,95,96], 86: [85,87,95,96,97], 87: [86,88,96,97,98]
}

cdef dict black_kpk_key_squares = {
    81: [22,32], 71: [22,32], 61: [22,32], 51: [22,32], 41: [22,32], 31: [22,32],
    88: [27,37], 78: [27,37], 68: [27,37], 58: [27,37], 48: [27,37], 38: [27,37],

    82: [61,62,63], 83: [62,63,64], 84: [63,64,65], 85: [64,65,66], 86: [65,66,67], 87: [66,67,68],
    72: [51,52,53], 73: [52,53,53], 74: [53,54,55], 75: [54,55,56], 76: [55,56,57], 77: [56,57,58],
    62: [41,42,43], 63: [42,43,44], 64: [43,44,45], 65: [44,45,46], 66: [45,46,47], 67: [46,47,48],

    52: [41,42,43,31,32,33], 53: [42,43,44,32,33,34], 54: [43,44,45,33,34,35],
    55: [44,45,46,34,35,36], 56: [45,46,47,35,36,37], 57: [46,47,48,36,37,38],

    42: [31,32,33,21,22,23], 43: [32,33,34,22,23,24], 44: [33,34,35,23,24,25],
    45: [34,35,36,24,25,26], 46: [35,36,37,25,26,27], 47: [36,37,38,26,27,28],

    32: [31,33,21,22,23], 33: [32,34,22,23,24], 34: [33,35,23,24,25],
    35: [34,36,24,25,26], 36: [35,37,25,26,27], 37: [36,38,26,27,28]
}

cdef int *white_pawn_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, 0, 0, 0, 0, 0, 0, 0, 0, ob,
    ob, 5, 10, 10, -20, -20, 10, 10, 5, ob,
    ob, 5, -5, -10, 0, 0, -10, -5, 5, ob,
    ob, 0, 0, 0, 20, 20, 0, 0, 0, ob,
    ob, 5, 5, 10, 25, 25, 10, 5, 5, ob,
    ob, 10, 10, 20, 30, 30, 20, 10, 10, ob,
    ob, 50, 50, 50, 50, 50, 50, 50, 50, ob,
    ob, 0, 0, 0, 0, 0, 0, 0, 0, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

cdef int *white_knight_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -50, -40, -30, -30, -30, -30, -40, -50, ob,
    ob, -40, -20, 0, 5, 5, 0, -20, -40, ob,
    ob, -30, 5, 10, 15, 15, 10, 5, -30, ob,
    ob, -30, 0, 15, 20, 20, 15, 0, -30, ob,
    ob, -30, 5, 15, 20, 20, 15, 5, -30, ob,
    ob, -30, 0, 10, 15, 15, 10, 0, -30, ob,
    ob, -40, -20, 0, 0, 0, 0, -20, -40, ob,
    ob, -50, -40, -30, -30, -30, -30, -40, -50, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

cdef int *white_bishop_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -20, -10, -10, -10, -10, -10, -10, -20, ob,
    ob, -10, 5, 0, 0, 0, 0, 5, -10, ob,
    ob, -10, 10, 10, 10, 10, 10, 10, -10, ob,
    ob, -10, 0, 10, 10, 10, 10, 0, -10, ob,
    ob, -10, 5, 5, 10, 10, 5, 5, -10, ob,
    ob, -10, 0, 5, 10, 10, 5, 0, -10, ob,
    ob, -10, 0, 0, 0, 0, 0, 0, -10, ob,
    ob, -20, -10, -10, -10, -10, -10, -10, 20, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

cdef int *white_rook_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, 0, 0, 0, 5, 5, 0, 0, 0, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, 5, 10, 10, 10, 10, 10, 10, 5, ob,
    ob, 0, 0, 0, 0, 0, 0, 0, 0, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

cdef int *white_queen_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -20, -10, -10, -5, -5, -10, -10, -20, ob,
    ob, -10, 0, 5, 0, 0, 0, 0, -10, ob,
    ob, -10, 5, 5, 5, 5, 5, 0, -10, ob,
    ob, 0, 0, 5, 5, 5, 5, 0, -5, ob,
    ob, -5, 0, 5, 5, 5, 5, 0, -5, ob,
    ob, -10, 0, 5, 5, 5, 5, 0, -10, ob,
    ob, -10, 0, 0, 0, 0, 0, 0, -10, ob,
    ob, -20, -10, -10, -5, -5, -10, -10, -20, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

# note -the page has a mid and end game, this is mid game, which makes it weak in end game
cdef int *white_king_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, 20, 30, 10, 0, 0, 10, 30, 20, ob,
    ob, 20, 20, 0, 0, 0, 0, 20, 20, ob,
    ob, -10, -20, -20, -20, -20, -20, -20, -10, ob,
    ob, -20, -30, -30, -40, -40, -30, -30, -20, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

cdef int *white_king_endgame_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -50, -30, -30, -30, -30, -30, -30, -50, ob,
    ob, -30, -30, 0, 0, 0, 0, -30, -30, ob,
    ob, -30, -10, 20, 30, 30, 20, -10, -30, ob,
    ob, -30, -10, 30, 40, 40, 30, -10, -30, ob,
    ob, -30, -10, 30, 40, 40, 30, -10, -30, ob,
    ob, -30, -10, 20, 30, 30, 20, -10, -30, ob,
    ob, -30, -20, -10, 0, 0, -10, -20, -30, ob,
    ob, -50, -40, -30, -20, -20, -30, -40, -50, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

cdef:
    # will initialize these programmatically later
    int black_pawn_pst[120]
    int black_knight_pst[120]
    int black_bishop_pst[120]
    int black_rook_pst[120]
    int black_queen_pst[120]
    int black_king_pst[120]
    int black_king_endgame_pst[120]

    dict piece_value_dict = {WP: 100, BP: 100, WN: 320, BN: 320, WB: 330, BB: 330, WR: 500, BR: 500,
                       WQ: 900, BQ: 900, WK: 20000, BK: 20000}

    list white_piece_list = [WP, WN, WB, WR, WQ, WK]
    list black_piece_list = [BP, BN, BB, BR, BQ, BK]

    dict piece_to_string_dict = {WP: "P", WN: "N", WB: "B", WR: "R", WQ: "Q", WK: "K", BP: "p", BN: "n", BB: "b", BR: "r",
                            BQ: "q", BK: "k", EMPTY: " ", OFF_BOARD: "x"}
    dict string_to_piece_dict = {"P": WP, "N": WN, "B": WB, "R": WR, "Q": WQ, "K": WK, "p": BP, "n": BN, "b": BB, "r": BR,
                           "q": BQ, "k": BK, " ": EMPTY, "x": OFF_BOARD}


def initialize_psts(is_debug=False):
    # Evaluation function stolen from https://chessprogramming.wikispaces.com/Simplified+evaluation+function

    # why am I doing this?  I could have added the value of pieces in the definition
    # and defined the black pst's above instead of doing programatically.  Reason is that
    # this way if I want to change the model slightly, I have to make the change in one place
    # and it will percolate elsewhere automatically, instead of changing potentially dozens
    # of values in multiple lists.

    # add the value of the pieces to the pst's

    for rank in range(90, 10, -10):
        for file in range(1, 9, 1):
            white_pawn_pst[rank + file] += piece_value_dict[WP]
            white_knight_pst[rank + file] += piece_value_dict[WN]
            white_bishop_pst[rank + file] += piece_value_dict[WB]
            white_rook_pst[rank + file] += piece_value_dict[WR]
            white_queen_pst[rank + file] += piece_value_dict[WQ]
            white_king_pst[rank + file] += piece_value_dict[WK]
            white_king_endgame_pst[rank + file] += piece_value_dict[WK]

    # to make the black pst's
    # rank 20 maps to rank 90
    # rank 30 maps to rank 80
    # rank 40 maps to rank 70
    # rank 50 maps to rank 60
    # etc.
    # rank 0,10,100,110 are off board

    # initialize off-board ranks
    for file in range(0, 10, 1):
        for rank in [0, 10, 100, 110]:
            black_pawn_pst[rank + file] = ob
            black_knight_pst[rank + file] = ob
            black_bishop_pst[rank + file] = ob
            black_rook_pst[rank + file] = ob
            black_queen_pst[rank + file] = ob
            black_king_pst[rank + file] = ob
            black_king_endgame_pst[rank + file] = ob

    for file in range(0, 10, 1):
        for rankflip in [(20, 90), (30, 80), (40, 70), (50, 60)]:
            black_pawn_pst[rankflip[0] + file] = -1 * white_pawn_pst[rankflip[1] + file]
            black_pawn_pst[rankflip[1] + file] = -1 * white_pawn_pst[rankflip[0] + file]
            black_knight_pst[rankflip[0] + file] = -1 * white_knight_pst[rankflip[1] + file]
            black_knight_pst[rankflip[1] + file] = -1 * white_knight_pst[rankflip[0] + file]
            black_bishop_pst[rankflip[0] + file] = -1 * white_bishop_pst[rankflip[1] + file]
            black_bishop_pst[rankflip[1] + file] = -1 * white_bishop_pst[rankflip[0] + file]
            black_rook_pst[rankflip[0] + file] = -1 * white_rook_pst[rankflip[1] + file]
            black_rook_pst[rankflip[1] + file] = -1 * white_rook_pst[rankflip[0] + file]
            black_queen_pst[rankflip[0] + file] = -1 * white_queen_pst[rankflip[1] + file]
            black_queen_pst[rankflip[1] + file] = -1 * white_queen_pst[rankflip[0] + file]
            black_king_pst[rankflip[0] + file] = -1 * white_king_pst[rankflip[1] + file]
            black_king_pst[rankflip[1] + file] = -1 * white_king_pst[rankflip[0] + file]
            black_king_endgame_pst[rankflip[0] + file] = -1 * white_king_endgame_pst[rankflip[1] + file]
            black_king_endgame_pst[rankflip[1] + file] = -1 * white_king_endgame_pst[rankflip[0] + file]

initialize_psts()


cdef class ChessPositionCache:

    cdef:
        public unsigned long whitetomove
        public unsigned long blacktomove
        public unsigned long whitecastleking
        public unsigned long whitecastlequeen
        public unsigned long blackcastleking
        public unsigned long blackcastlequeen

        # public list enpassanttarget, whitep, blackp, whiten, blackn
        public unsigned long long enpassanttarget[120]
        public unsigned long long whitep[120]
        public unsigned long long blackp[120]
        public unsigned long long whiten[120]
        public unsigned long long blackn[120]
        # public list whiteb, blackb, whiter, blackr, whiteq, blackq, whitek, blackk
        public unsigned long long whiteb[120]
        public unsigned long long blackb[120]
        public unsigned long long whiter[120]
        public unsigned long long blackr[120]
        public unsigned long long whiteq[120]
        public unsigned long long blackq[120]
        public unsigned long long whitek[120]
        public unsigned long long blackk[120]
        public dict board_mask_dict
        public list cache

        public long long inserts, probe_hits

        public unsigned long cachesize


    def __init__(self, cachesize=1048799):   # 1,299,827 is prime as is 251,611. 1,048,799 is smallest prime > 2^20

        cdef int i, square

        self.whitetomove = random.getrandbits(64)
        self.blacktomove = random.getrandbits(64)
        self.whitecastleking = random.getrandbits(64)
        self.whitecastlequeen = random.getrandbits(64)
        self.blackcastleking = random.getrandbits(64)
        self.blackcastlequeen = random.getrandbits(64)

        for i in range(120):
            # this could be 64, but I don't want to have to map a 120-square representation to a 64-square representation
            # when computing a hash.  And enpassanttarget can only be one of 16 squares.
            self.whitep[i] = <unsigned long long>random.getrandbits(64)
            self.blackp[i] = <unsigned long long>random.getrandbits(64)
            self.whiten[i] = <unsigned long long>random.getrandbits(64)
            self.blackn[i] = <unsigned long long>random.getrandbits(64)
            self.whiteb[i] = <unsigned long long>random.getrandbits(64)
            self.blackb[i] = <unsigned long long>random.getrandbits(64)
            self.whiter[i] = <unsigned long long>random.getrandbits(64)
            self.blackr[i] = <unsigned long long>random.getrandbits(64)
            self.whiteq[i] = <unsigned long long>random.getrandbits(64)
            self.blackq[i] = <unsigned long long>random.getrandbits(64)
            self.whitek[i] = <unsigned long long>random.getrandbits(64)
            self.blackk[i] = <unsigned long long>random.getrandbits(64)
            self.enpassanttarget[i] = <unsigned long long>random.getrandbits(64)



        self.board_mask_dict = {WP: self.whitep, BP: self.blackp, WN: self.whiten, BN: self.blackn, WB: self.whiteb, BB: self.blackb,
                                WR: self.whiter, BR: self.blackr, WQ: self.whiteq, BQ: self.blackq, WK: self.whitek, BK: self.blackk}

        self.cachesize = cachesize
        self.cache = [None] * cachesize


    cdef unsigned long long compute_hash(self, board):
        cdef:
            unsigned long hash = 0
            int piece, i

        if board.board_attributes & W_TO_MOVE:
            hash = self.whitetomove
        else:
            hash = self.blacktomove

        if board.board_attributes & W_CASTLE_KING:
            hash ^= self.whitecastleking
        if board.board_attributes & W_CASTLE_QUEEN:
            hash ^= self.whitecastlequeen
        if board.board_attributes & B_CASTLE_QUEEN:
            hash ^= self.blackcastlequeen
        if board.board_attributes & B_CASTLE_KING:
            hash ^= self.blackcastleking
        if board.en_passant_target_square:
            hash ^= self.enpassanttarget[board.en_passant_target_square]

        for piece in [BP, BN, BB, BR, BQ, BK, WP, WN, WB, WR, WQ, WK]:
            for i in range(1, board.piece_count_locations[piece][0]+1):
                square = board.piece_count_locations[piece][i]
                hash ^= self.board_mask_dict[piece][square]

        return hash

    cdef debug_analyze_hash_differences(self, hash, bch):
        print("Actual hash: ", hash)
        print("Cached hash: ", bch)
        if bch ^ self.whitetomove == hash:
            print ("Off by white to move")
        elif bch ^ self.blacktomove == hash:
            print ("Off by black to move")
        elif (bch ^ self.whitetomove) ^ self.blacktomove == hash:
            print ("off by both to move")
        elif bch ^ self.whitecastleking == hash:
            print ("off by wck")
        elif bch ^ self.whitecastlequeen == hash:
            print ("off by wcq")
        elif (bch ^ self.whitecastleking) ^ self.whitecastlequeen == hash:
            print ("off by wc")
        elif bch ^ self.blackcastleking == hash:
            print ("off by bck")
        elif bch ^ self.blackcastlequeen == hash:
            print ("off by bcq")
        elif (bch ^ self.blackcastleking) ^ self.blackcastlequeen == hash:
            print ("off by bc")
        else:
            for i in range(120):
                if bch ^ self.enpassanttarget[i] == hash:
                    print ("off by en passant", i)
                    break
                else:
                    for piece in [BP, BN, BB, BR, BQ, BK, WP, WN, WB, WR, WQ, WK]:
                        if bch ^ self.board_mask_dict[piece][i] == hash:
                            print ("off by ", piece, " ", i)
                            break
        print ("analysis completed")


    cdef insert(self, ChessBoard board, tuple stuff):
        # We use "replace always."

        cdef:
            unsigned long long hash
            unsigned long hash_modded

        hash = board.cached_hash
        hash_modded = hash % self.cachesize
        self.cache[hash_modded] = (hash, stuff)


    cdef tuple probe(self, ChessBoard board):
        # Returns the "stuff" that was cached if board exactly matches what is in the cache.
        # After testing 2 caches, one based on "depth" and "age" and the other "always replace," found that
        # We spent much more time in maintaining the two caches than we saved by having the additional information.
        # This is consistent with other tools with caches of the size we have.  The 2-cache strategy did better
        # when dealing with lots more collisions and smaller caches.

        cdef:
            unsigned long long hash
            unsigned long hash_modded
            tuple c


        hash = board.cached_hash
        hash_modded = hash % self.cachesize
        c = self.cache[hash_modded]
        if c is None:
            return None
        elif hash == c[0]:
            return c[1]
        else:
            return None

cdef ChessPositionCache TRANSPOSITION_TABLE = ChessPositionCache()


cdef str pretty_print_move(Move move, is_debug=False, is_san=False):

    cdef:
        int start = move & START
        int end = ((move & END) >> END_SHIFT)
        int piece_moving = ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT)
        int piece_captured = ((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT)
        int capture_differential = ((move & CAPTURE_DIFFERENTIAL) >> CAPTURE_DIFFERENTIAL_SHIFT) - CAPTURE_DIFFERENTIAL_OFFSET
        int promoted_to = ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT)
        int flags = ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT)

        str tmpmove = ""

    if move == NULL_MOVE:
        return "{END}"

    if not flags & MOVE_CASTLE:
        if not is_san:
            tmpmove = arraypos_to_algebraic(start)
            if not piece_captured:
                tmpmove += "-"
        else:
            if not(piece_moving & PAWN):
                tmpmove = piece_to_string_dict[piece_moving].upper()
            elif piece_captured:
                tmpmove = arraypos_to_algebraic(start)[0]

        if piece_captured:
            tmpmove += "x"
        tmpmove += arraypos_to_algebraic(end)
        if promoted_to:
            tmpmove += " (%s)" % piece_to_string_dict[promoted_to]
    else:
        if end > start:
            tmpmove = "O-O"
        else:
            tmpmove = "O-O-O"

    if flags & MOVE_CHECK:
        tmpmove += "+"

    if is_debug:
        if flags & MOVE_DOUBLE_PAWN:
            tmpmove += " 2 square pawn move"
        if piece_captured:
            tmpmove += " %s captured, differential = %d " % (piece_to_string_dict[piece_captured],
                                                             capture_differential)
            if flags & MOVE_EN_PASSANT:
                tmpmove += " en passant."
            else:
                tmpmove += "."

    return tmpmove


cdef Move return_validated_move(board, str algebraic_move):
    """

    :param board: chessboard with the position in it
    :param algebraic_move: the move in a2-a4 style format
    :return: None if the move is invalid, else the ChessMove object corresponding to the move.
    """

    cdef:
        int start_pos, end_pos, i
        ChessMoveListGenerator move_list
        Move retval

    # out of habit, I type moves e.g. e2-e4, but the xboard protocol uses "e2e4" without the hyphen.
    # I want the game to be tolerant of both.
    if algebraic_move.find("-") == 2:
        algebraic_move = algebraic_move[0:2] + algebraic_move[3:]

    try:
        assert (len(algebraic_move) in [4, 5])
        assert(algebraic_move[0] in ["a", "b", "c", "d", "e", "f", "g", "h"])
        assert(algebraic_move[2] in ["a", "b", "c", "d", "e", "f", "g", "h"])
        assert(algebraic_move[1] in ["1", "2", "3", "4", "5", "6", "7", "8"])
        assert(algebraic_move[3] in ["1", "2", "3", "4", "5", "6", "7", "8"])
    except:
        return NULL_MOVE

    start_pos = algebraic_to_arraypos(algebraic_move[0:2])
    end_pos = algebraic_to_arraypos(algebraic_move[2:4])
    move_list = ChessMoveListGenerator(board)
    retval = NULL_MOVE

    move_list.generate_move_list()
    for move in move_list.move_list:
        if (move & START) == start_pos and ((move & END) >> END_SHIFT) == end_pos:
            retval = move
            if (retval & PROMOTED_TO):
                if len(algebraic_move) == 4:
                    print("# promotion not provided - assuming queen.")
                    promotion = "q"
                else:
                    promotion = algebraic_move[4]
                if board.board_attributes & W_TO_MOVE:
                    promotion = promotion.upper()
                else:
                    promotion = promotion.lower()
                retval &= (~PROMOTED_TO)
                retval |= ((<long long>string_to_piece_dict[promotion] << PROMOTED_TO_SHIFT))
                break
    return retval


# Data structure to try to speed up evaluation of whether or not a position is a check.
# Based on the move tables that GNU Chess uses.

"""
We are going to make an array of board positions, so the positions from 21..28, 31..38, ... , 91..98 are the
interesting ones.  At each cell will be a list of tuples in the following format:

CELL TO CHECK, PIECES IN THAT CELL THAT COULD PUT IN CHECK, WHERE TO GO IN LIST NEXT IF CELL BLOCKED.

If next cell is not blocked, just keep going ahead in the list and interpret the next tuple.  If you get to
None then there is no check.

So let's look at the entry for position 21 (square a1).  A white king in a1 can be in check by a rook or queen on the a file
or first rank, so positions 31,41,51,...,91 or positions 22,23,24,...,28.  King can be in check by a bishop or queen on
the diagonal a1, b2, c3... which is positions 32,43,54,..., 98.  Can be in check by a knight in b3 or c2, so squares
42 or 33, or a pawn in b2 which is position 32.  All the adjacent squares a king could put you in check, which cannot
happen in a game but we want to prevent illegal moves so we test for that too.  So we would have a list that would look
like:

(31, ROOK | QUEEN | KING, 7)   # This is tuple 0
(41, ROOK | QUEEN , 7)
(51, ROOK | QUEEN , 7)
(61, ROOK | QUEEN , 7)
(71, ROOK | QUEEN , 7)
(81, ROOK | QUEEN , 7)
(91, ROOK | QUEEN , 7)
(22, ROOK | QUEEN | KING, 14)  # This is tuple 7
(23, ROOK | QUEEN , 14)
(24, ROOK | QUEEN , 14)
(25, ROOK | QUEEN , 14)
(26, ROOK | QUEEN , 14)
(27, ROOK | QUEEN , 14)
(28, ROOK | QUEEN , 14)
(32, BISHOP | QUEEN | KING | PAWN, 21)  # This is tuple 14
(43, BISHOP | QUEEN, 21)
(54, BISHOP | QUEEN, 21)
(65, BISHOP | QUEEN, 21)
(76, BISHOP | QUEEN, 21)
(87, BISHOP | QUEEN, 21)
(98, BISHOP | QUEEN, 21)
(42, KNIGHT, 22)   # This is Tuple 21
(33, KNIGHT, None) # This is Tuple 22
(None, None, None)

Take first tuple.  Look at the piece in position 31.  If no piece, move to the second tuple.  If there is a piece,
if it's a black Rook, Queen, or King then it is a check so exit.  If it's any other piece, jump ahead to Tuple 7,
because it's blocked.  In the knight list, you move to the next tuple in the list whether square is empty or occupied
by a non-Knight piece, because knights don't get blocked by pieces.  If you end up at None, then you have no check.
Will need two of these tables - one for white and the other for black - because pawns move differently.

CYTHON ONLY - this code moved to check_tables.pxd as it is now generated into a 3-d array instead of a list of lists of tuples

"""



cdef class ChessMoveListGenerator:

    cdef public ChessBoard board
    cdef public list move_list

    def __init__(self, ChessBoard board=None):
        self.move_list = []
        if board is None:
            self.board = ChessBoard()
        else:
            self.board = board

    def pretty_print(self):
        outstr = ""
        for move in self.move_list:
            outstr += pretty_print_move(move, is_san=True, is_debug=True) + ","
        outstr += "\n"
        return outstr

    cdef list generate_direction_moves(self, int start_pos, int velocity, int perpendicular_velocity, int piece_moving):
        """

        :param start_pos:
        :param velocity: -1 would be west, +10 north, +11 northwest, etc
        :param perpendicular_velocity: to save me an if test, the direction 90 degrees from velocity
        :param piece_moving: character representing the piece so we can attach it to the move
        :return: list of move tuples
        """
        cdef:
            list ret_list = [], dirlist
            int cur_pos = start_pos + velocity
            int direction, i, listlen,j
            int testpos, blocker
            int *queen_delta = [1, -1, 10, -10, 11, -11, 9, -9]
            int *rookbishop_delta = [velocity, perpendicular_velocity, -1 * perpendicular_velocity]
            int arrayrange
            int *array_to_use
            Move move

        if self.board.board_attributes & W_TO_MOVE:
            enemy_list = black_piece_list
            enemy_king = BK
        else:
            enemy_list = white_piece_list
            enemy_king = WK

        # add all the blank squares in that direction
        while self.board.board_array[cur_pos] == EMPTY:
            ret_list.append(create_move(start_pos, cur_pos, piece_moving, 0, 0, 0, 0))
            cur_pos += velocity

        # if first non-blank square is the opposite color, it is a capture
        blocker = self.board.board_array[cur_pos]

        if blocker in enemy_list:
            capture_diff = piece_value_dict[blocker] - piece_value_dict[piece_moving]
            ret_list.append(create_move(start_pos, cur_pos, piece_moving, blocker, capture_diff, 0, 0))

        if piece_moving & QUEEN:
            array_to_use = queen_delta
            arrayrange = 8

        else:
            # Two ways for the move to be a check.  First, we take the piece that was blocking us from check,
            # so look straight ahead.  Then, look perpendicular.  Cannot put the king into check behind us, else
            # king would have already been in check.
            array_to_use = rookbishop_delta
            arrayrange = 3

        listlen = len(ret_list)
        for i in range(listlen):
            move = ret_list[i]
            for j in range(arrayrange):
                direction = array_to_use[j]
                # The queen array has all 8 directions, but we do not need to consider the direction she was
                # moving from, since the king can't be in check that way.  The rookbishop delta already excluded
                # that direction.
                if direction != (-1 * velocity):
                    testpos = ((move & END) >> END_SHIFT) + direction
                    while self.board.board_array[testpos] == EMPTY:
                        testpos += direction
                    if self.board.board_array[testpos] == enemy_king:
                        move |= (<long long>MOVE_CHECK << MOVE_FLAGS_SHIFT)
                        ret_list[i] = move
                        break

        return ret_list

    cdef list generate_slide_moves(self, int start_pos, int piece_moving):

        cdef:
            list north_list, west_list, east_list, south_list

        north_list = self.generate_direction_moves(start_pos, 10, 1, piece_moving)
        west_list = self.generate_direction_moves(start_pos, -1, 10, piece_moving)
        east_list = self.generate_direction_moves(start_pos, 1, 10, piece_moving)
        south_list = self.generate_direction_moves(start_pos, -10, 1, piece_moving)

        return north_list + west_list + east_list + south_list

    cdef list generate_diagonal_moves(self, int start_pos, int piece_moving):

        cdef:
            list nw_list, ne_list, sw_list, se_list

        nw_list = self.generate_direction_moves(start_pos, 9, 11, piece_moving)
        ne_list = self.generate_direction_moves(start_pos, 11, 9, piece_moving)
        sw_list = self.generate_direction_moves(start_pos, -11, 9, piece_moving)
        se_list = self.generate_direction_moves(start_pos, -9, 11, piece_moving)

        return nw_list + ne_list + sw_list + se_list

    cdef list generate_knight_moves(self, int start_pos):

        cdef:
            list ret_list = []
            list enemy_list
            int knight = self.board.board_array[start_pos]
            int piece, destpos, pos, testpos, targetsquare, capture_diff, listlen, i, j
            int *delta = [-21, -19, -12, -8, 21, 19, 12, 8]
            Move move

        if knight & BLACK:  # quicker than referencing the white_to_move in the board object
            enemy_list = white_piece_list
        else:
            enemy_list = black_piece_list

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for j in range(8):
            dest_pos = start_pos + delta[j]
            if self.board.board_array[dest_pos] == EMPTY:
                ret_list.append(create_move(start_pos, dest_pos, knight, 0, 0, 0, 0))
            else:
                piece = self.board.board_array[dest_pos]
                if piece in enemy_list:
                    capture_diff = piece_value_dict[piece] - piece_value_dict[knight]
                    ret_list.append(create_move(start_pos, dest_pos, knight, piece, capture_diff, 0, 0))

        listlen = len(ret_list)
        for i in range(listlen):
            move = ret_list[i]
            pos = (move & END) >> END_SHIFT
            for j in range(8):

                targetsquare = self.board.board_array[pos+delta[j]]
                if ((knight ^ targetsquare) & BLACK) and (targetsquare & KING):
                    move |= (<long long>MOVE_CHECK << MOVE_FLAGS_SHIFT)
                    ret_list[i] = move
                    break

        return ret_list


    cdef bint test_for_check_after_castle(self, int rook_pos, list directions, int enemy_king):

        cdef:
            int direction, testpos

        # I could figure directions and enemy_king out from rook_pos, but faster execution to hard code in the caller
        for direction in directions:
            testpos = rook_pos + direction
            while self.board.board_array[testpos] == EMPTY:
                testpos += direction
            if self.board.board_array[testpos] == enemy_king:
                return True
        return False


    cdef list generate_king_moves(self, int start_pos, bint currently_in_check):
        cdef:
            list ret_list = []
            int king = self.board.board_array[start_pos]
            int destpos, piece, capture_diff, flags
            int *delta = [-1, 9, 10, 11, 1, -9, -10, -11]
            int i

        if king & BLACK:  # quicker than referencing the white_to_move in the board object
            enemy_list = white_piece_list
        else:
            enemy_list = black_piece_list

        for i in range(8):
            dest_pos = start_pos + delta[i]
            if self.board.board_array[dest_pos] == EMPTY:
                ret_list.append(create_move(start_pos, dest_pos, king, 0, 0, 0, 0))
            else:
                piece = self.board.board_array[dest_pos]
                if piece in enemy_list:
                    capture_diff = piece_value_dict[piece] - piece_value_dict[king]
                    ret_list.append(create_move(start_pos, dest_pos, king, piece, capture_diff, 0, 0))

        if not currently_in_check:
            if king == WK and start_pos == 25:
                # arraypos 25 = "e1"
                if self.board.board_attributes & W_CASTLE_KING:
                    if (self.board.board_array[26] == EMPTY and self.board.board_array[27] == EMPTY
                            and self.board.board_array[28] == WR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(26, [-1, 10], BK):
                            flags |= MOVE_CHECK
                        ret_list.append(create_move(25, 27, king, 0, 0, 0, flags))
                if self.board.board_attributes & W_CASTLE_QUEEN:
                    if (self.board.board_array[24] == EMPTY and self.board.board_array[23] == EMPTY
                            and self.board.board_array[22] == EMPTY and self.board.board_array[21] == WR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(24, [1, 10], BK):
                            flags |= MOVE_CHECK
                        ret_list.append(create_move(25, 23, king, 0, 0, 0, flags))
            elif king == BK and start_pos == 95:
                # arraypos 95 = "e8"
                if self.board.board_attributes & B_CASTLE_KING:
                    if (self.board.board_array[96] == EMPTY and self.board.board_array[97] == EMPTY
                            and self.board.board_array[98] == BR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(96, [-1, -10], WK):
                            flags |= MOVE_CHECK
                        ret_list.append(create_move(95, 97, king, 0, 0, 0, flags))
                if self.board.board_attributes & B_CASTLE_QUEEN:
                    if (self.board.board_array[94] == EMPTY and self.board.board_array[93] == EMPTY
                            and self.board.board_array[92] == EMPTY and self.board.board_array[91] == BR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(94, [1, -10], WK):
                            flags |= MOVE_CHECK
                        ret_list.append(create_move(95, 93, king, 0, 0, 0, flags))

        return ret_list

    cdef list generate_pawn_moves(self, int start_pos):

        cdef:
            list ret_list = []
            int pawn, enemypawn, normal_move, double_move, capture_list, capture_right, end
            list promotion_list, enemy_list
            int start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max, promotion
            int dest_square, dest_pos, capture_diff, piece_captured, listlen, i
            Move move
            list x


        if self.board.board_attributes & W_TO_MOVE:
            pawn, enemypawn = WP, BP
            normal_move, double_move, capture_left, capture_right = (10, 20, 9, 11)
            promotion_list = [WQ, WN, WR, WB]
            enemy_list = black_piece_list
            start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max = (31, 38, 81, 88)
        else:
            pawn, enemypawn = BP, WP
            normal_move, double_move, capture_left, capture_right = (-10, -20, -9, -11)
            promotion_list = [BQ, BN, BR, BB]
            enemy_list = white_piece_list
            start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max = (81, 88, 31, 38)

        if self.board.board_array[start_pos + normal_move] == EMPTY:
            if penultimate_rank_min <= start_pos <= penultimate_rank_max:
                for promotion in promotion_list:
                    ret_list.append(create_move(start_pos, start_pos+normal_move, pawn, 0, 0, promotion, 0))
            else:
                ret_list.append(create_move(start_pos, start_pos+normal_move, pawn, 0, 0, 0, 0))
        if ((start_rank_min <= start_pos <= start_rank_max) and
                    self.board.board_array[start_pos + normal_move] == EMPTY and
                    self.board.board_array[start_pos + double_move] == EMPTY):
            ret_list.append(create_move(start_pos, start_pos+double_move, pawn, 0, 0, 0, MOVE_DOUBLE_PAWN))

        for dest_pos in [start_pos + capture_left, start_pos + capture_right]:
            dest_square = self.board.board_array[dest_pos]
            if (dest_square in enemy_list or
                        dest_pos == self.board.en_passant_target_square):
                if dest_pos == self.board.en_passant_target_square:
                    ret_list.append(create_move(start_pos, dest_pos, pawn, enemypawn, 0, 0, MOVE_EN_PASSANT))
                else:
                    piece_captured = dest_square
                    capture_diff = piece_value_dict[piece_captured] - piece_value_dict[pawn]
                    if penultimate_rank_min <= start_pos <= penultimate_rank_max:
                        for promotion in promotion_list:
                            ret_list.append(create_move(start_pos, dest_pos, pawn, piece_captured, capture_diff, promotion, 0))
                    else:
                        ret_list.append(create_move(start_pos, dest_pos, pawn, piece_captured, capture_diff, 0, 0))

        # test moves for check - faster than running side_to_move_in_check later
        listlen = len(ret_list)
        for i in range(listlen):
            move = ret_list[i]
            end = (move & END) >> END_SHIFT
            targetleft = self.board.board_array[end+capture_left]
            targetright = self.board.board_array[end+capture_right]
            # if pawn is a different color than target left and target left is a king...
            if ((pawn ^ targetleft) & BLACK and targetleft & KING) or ((pawn ^ targetright) & BLACK and targetright & KING):
                move |= (<long long>MOVE_CHECK << MOVE_FLAGS_SHIFT)
                ret_list[i] = move

        for move in ret_list:
            if move & (<long long>KING << PIECE_CAPTURED_SHIFT):
                for x in self.board.move_history:
                    print(pretty_print_move(x[0]))
                raise ValueError("CAPTURED KING - preceding move not detected as check or something")

        return ret_list

    cdef generate_move_list(self, Move last_best_move=NULL_MOVE, Move killer_move1 = NULL_MOVE, Move killer_move2 = NULL_MOVE):
        """

        :param last_best_move: optional - if provided, was the last known best move for this position, and will end up
                first in the return list.
        :return: Updates the move_list member to the moves in order they should be searched.  Current heuristic is
                1) last_best_move goes first if present
                2) Captures go next, in order of MVV-LVA - most valuable victim minus least valuable aggressor
                3) Killer Moves come next
                4) Any moves that put the opponent in check
                5) Any other moves
        """
        cdef:
            list potential_list, capture_list, noncapture_list, check_list, priority_list, killer_list
            list pinned_piece_list, discovered_check_list
            Move move, m
            int pos, en_passant_target_square,i
            bint currently_in_check, is_king_move, is_pawn_move, move_valid
            int pawn, knight, bishop, rook, queen, king, piece, piece_moving
            int which_king_moving, which_rook_moving
            int start, end, piece_captured, promoted_to, flags, middle_square

        self.move_list = []
        potential_list = []
        capture_list = []
        noncapture_list = []
        check_list = []
        priority_list = []
        killer_list = []

        pinned_piece_list = self.board.generate_pinned_list(True)
        discovered_check_list = self.board.generate_pinned_list(False)

        currently_in_check = self.board.board_attributes & BOARD_IN_CHECK
        en_passant_target_square = self.board.en_passant_target_square

        if self.board.board_attributes & W_TO_MOVE:
            pawn, knight, bishop, rook, queen, king = WP, WN, WB, WR, WQ, WK
        else:
            pawn, knight, bishop, rook, queen, king = BP, BN, BB, BR, BQ, BK

        for i in range(1, self.board.piece_count_locations[pawn][0] + 1):
            piece = self.board.piece_count_locations[pawn][i]
            potential_list += self.generate_pawn_moves(piece)
        for i in range(1, self.board.piece_count_locations[knight][0] + 1):
            piece = self.board.piece_count_locations[knight][i]
            # pinned knights can't move
            if piece not in pinned_piece_list:
                potential_list += self.generate_knight_moves(piece)
        for i in range(1, self.board.piece_count_locations[bishop][0] + 1):
            piece = self.board.piece_count_locations[bishop][i]
            potential_list += self.generate_diagonal_moves(piece, bishop)
        for i in range(1, self.board.piece_count_locations[rook][0] + 1):
            piece = self.board.piece_count_locations[rook][i]
            potential_list += self.generate_slide_moves(piece, rook)
        for i in range(1, self.board.piece_count_locations[queen][0] + 1):
            piece = self.board.piece_count_locations[queen][i]
            potential_list += self.generate_diagonal_moves(piece, queen)
            potential_list += self.generate_slide_moves(piece, queen)

        potential_list += self.generate_king_moves(self.board.piece_count_locations[king][1], currently_in_check)

        for move in potential_list:
            start = move & START
            end = (move & END) >> END_SHIFT
            piece_captured = (move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT
            promoted_to = (move & PROMOTED_TO) >> PROMOTED_TO_SHIFT
            flags = (move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT

            piece_moving = self.board.board_array[start]
            is_king_move = piece_moving & KING
            is_pawn_move = piece_moving & PAWN

            move_valid = True  # assume it is

            try:
                self.board.apply_move(move)
            except:
                print(self.board.pretty_print(False))
                print("Trying: %d %d %d %d %d" % (start, end, piece_captured, promoted_to, flags))
                print(pretty_print_move(move, True))
                raise



            # optimization: only positions where you could move into check are king moves,
            # moves of pinned pieces, or en-passant captures (because could remove two pieces
            # blocking king from check)
            if (currently_in_check or (start in pinned_piece_list) or is_king_move or
                    (end == en_passant_target_square and is_pawn_move)):

                self.board.board_attributes ^= W_TO_MOVE  # apply_moved flipped sides, so flip it back

                if self.board.side_to_move_is_in_check():
                    # if the move would leave the side to move in check, the move is not valid
                    move_valid = False
                elif flags & MOVE_CASTLE:
                    # cannot castle through check
                    # kings in all castles move two spaces, so find the place between the start and end,
                    # put the king there, and then test for check again

                    middle_square = (start + end) // 2
                    which_king_moving = self.board.board_array[end]
                    which_rook_moving = self.board.board_array[middle_square]

                    self.board.board_array[middle_square] = self.board.board_array[end]
                    self.board.piece_count_locations[which_king_moving][1] = middle_square

                    self.board.board_array[end] = EMPTY
                    if self.board.side_to_move_is_in_check():
                        move_valid = False

                    # put the king and rook back where they would belong so that unapply move works properly
                    self.board.board_array[middle_square] = which_rook_moving
                    self.board.board_array[end] = which_king_moving
                    self.board.piece_count_locations[which_king_moving][1] = end

                self.board.board_attributes ^= W_TO_MOVE  # flip it to the side whose turn it really is

            if piece_captured & KING:
                for x in self.board.move_history:
                    print(pretty_print_move(x[0]))
                raise ValueError("CAPTURED KING - preceding move not detected as check or something")

            if move_valid:
                if (start in discovered_check_list) or promoted_to or flags & MOVE_EN_PASSANT:
                    # we tested for all other checks when we generated the move
                    if self.board.side_to_move_is_in_check():
                        move |= (<long long>MOVE_CHECK << MOVE_FLAGS_SHIFT)
                        flags |= MOVE_CHECK
                if last_best_move == move:
                    priority_list = [move]
                elif piece_captured:
                    capture_list += [move]
                elif move == killer_move1 or move == killer_move2:
                    killer_list += [move]
                elif flags & MOVE_CHECK:
                    check_list += [move]
                else:
                    noncapture_list += [move]

            self.board.unapply_move()

        capture_list.sort(key=lambda mymove: -1 * (mymove & CAPTURE_DIFFERENTIAL))
        self.move_list = priority_list + capture_list + killer_list + check_list + noncapture_list


cdef class ChessBoard:

    cdef:
        public unsigned char board_array[120]
        public unsigned int board_attributes
        public int en_passant_target_square
        public int halfmove_clock
        public int fullmove_number
        public str cached_fen
        public unsigned long long cached_hash
        public list move_history

        # This is a very sparse array.  Most cells are not used.  The 1, 2, 4, 8, 16, 32 for white pieces, and
        # 65, 66, 68, 72, 80, 96 for black pieces are used, as those are the bit flags for the pieces themselves.
        # There can be a max of 10 of any one piece - assume that all 8 pawns promoted to the same of knight,
        # bishop, or rook and the original of those pieces were not captured.  The 0th cell is the piece count
        # for the given piece type.  The cells 1-10 are used for the pieces.  Cell 11 will always have a zero, so that
        # when we delete from a row in the array, we can always copy position n+1 into position n to "remove" from the
        # "list."  Originally there was a dictionary for piece count and piece locations, but
        # accessing Python objects is slow in Cython, and this will be faster.
        public int piece_count_locations[97][12]

    def __init__(self):
        cdef int i, j

        for i in range(120):
            self.board_array[i] = EMPTY

        # Originally, I had nice member variables for these.  However due to performance I'm trying to simplify
        # data storage.
        self.board_attributes = W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN | W_TO_MOVE

        self.en_passant_target_square = 0
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.cached_fen = ""
        self.cached_hash = 0
        self.move_history = []

        for i in range(97):
            for j in range(12):
                self.piece_count_locations[i][j] = 0
        self.erase_board()


    cdef void add_piece_to_location_array(self, int piece, int location):
        cdef int old_count
        old_count = self.piece_count_locations[piece][0]
        self.piece_count_locations[piece][old_count + 1] = location
        self.piece_count_locations[piece][0] = old_count + 1


    cdef void remove_piece_from_location_array(self, int piece, int location):
        cdef:
            int old_count, i
            bint found_it = False

        # we walk through the array starting in position 1, which would be where the first possible location
        # would be.  Until we find the piece we're trying to remove, we move forward in the array.  Once
        # we've found it, then we copy the item in position i+1 into position i, which eventually ends up with a
        # zero in what used to be the maximum position for the piece.  In the corner case where there had been
        # 10 of a piece, using cells 1-10, we still have cell 11, which is always 0, to copy back into cell 10.

        old_count = self.piece_count_locations[piece][0]
        for i in range(1, old_count + 1):
            if self.piece_count_locations[piece][i] == location:
                found_it = True
            if found_it:
                self.piece_count_locations[piece][i] = self.piece_count_locations[piece][i+1]
        self.piece_count_locations[piece][0] = old_count - 1

        if not found_it:
             raise IndexError("Cannot remove piece {:d} from location {:d}".format(piece,location))

    def erase_board(self):
        global TRANSPOSITION_TABLE

        TRANSPOSITION_TABLE = ChessPositionCache()

        for square in range(120):
            if arraypos_is_on_board(square):
                self.board_array[square] = EMPTY
            else:
                self.board_array[square] = OFF_BOARD

        self.board_attributes = W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN | W_TO_MOVE
        self.en_passant_target_square = 0
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.move_history = []
        self.initialize_piece_locations()
        self.cached_fen = self.convert_to_fen()
        self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)
        init_historical_hash_array()

    def initialize_start_position(self):
        global TRANSPOSITION_TABLE

        self.erase_board()
        # NOTE - you are looking at a mirror image up/down of the board below, the first line is the bottom of the board
        for square in range(120):
            if arraypos_is_on_board(square):
                if 31 <= square <= 38:
                    self.board_array[square] = WP
                elif 81 <= square <= 88:
                    self.board_array[square] = BP
                elif square in [21, 28]:
                    self.board_array[square] = WR
                elif square in [22, 27]:
                    self.board_array[square] = WN
                elif square in [23, 26]:
                    self.board_array[square] = WB
                elif square == 24:
                    self.board_array[square] = WQ
                elif square == 25:
                    self.board_array[square] = WK
                elif square in [91, 98]:
                    self.board_array[square] = BR
                elif square in [92, 97]:
                    self.board_array[square] = BN
                elif square in [93, 96]:
                    self.board_array[square] = BB
                elif square == 94:
                    self.board_array[square] = BQ
                elif square == 95:
                    self.board_array[square] = BK

        self.board_attributes = W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN | W_TO_MOVE
        self.initialize_piece_locations()
        self.cached_fen = self.convert_to_fen()
        self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)

    cdef initialize_piece_locations(self):

        cdef int i, j, rank, file, piece

        for i in range(97):
            for j in range(12):
                self.piece_count_locations[i][j] = 0

        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank+file]
                if piece:
                    self.add_piece_to_location_array(piece, rank+file)

    def pretty_print(self, in_color=True):

        colorama.init()

        outstr = ""
        for i in range(9, 1, -1):
            outstr += str(i-1) + ": "
            for j in range(1, 9, 1):

                # if exactly one of the two digits of the number i+j are even, then it is a black square.
                # if both are odd, or both are even, it is a white square
                piece = self.board_array[(10*i) + j]

                if in_color:
                    if xor(i % 2 == 0, j % 2 == 0):
                        outstr += colorama.Back.BLACK
                    else:
                        outstr += colorama.Back.WHITE

                    if piece in (BP, BN, BB, BR, BQ, BK):
                        outstr += colorama.Fore.BLUE
                    else:
                        outstr += colorama.Fore.GREEN

                    outstr += " " + piece_to_string_dict[piece] + " "
                else:
                    if piece == EMPTY:
                        outstr += "."
                    else:
                        outstr += piece_to_string_dict[piece]

            if in_color:
                outstr += colorama.Style.RESET_ALL
            outstr += "\n"
        if in_color:
            outstr += "    a  b  c  d  e  f  g  h\n"
        else:
            outstr += "   abcdefgh\n"
        return outstr

    def load_from_fen(self, fen):
        global TRANSPOSITION_TABLE, HISTORICAL_HASH_ARRAY, HISTORICAL_HASH_POSITION, HISTORICAL_HASH_MIN_POSITION

        self.erase_board()

        # The FEN loads from a8 through h8, then a7 through h7, etc.
        cur_square = algebraic_to_arraypos("a8")

        for counter in range(fen.find(" ")):
            cur_char = fen[counter]
            if cur_char == "/":
                # move to the next rank
                cur_square -= 10
                # now move to the first file in this rank
                cur_square = (10 * (cur_square // 10)) + 1  # if this operation happens often, refactor into function.
            elif "1" <= cur_char <= "8":
                cur_square += int(cur_char)
            else:
                self.board_array[cur_square] = string_to_piece_dict[cur_char]
                cur_square += 1

        self.board_attributes = 0
        counter = fen.find(" ") + 1
        if fen[counter] == "w":
            self.board_attributes |= W_TO_MOVE

        counter += 2
        while fen[counter] != " ":
            if fen[counter] == "K":
                self.board_attributes |= W_CASTLE_KING
            elif fen[counter] == "Q":
                self.board_attributes |= W_CASTLE_QUEEN
            elif fen[counter] == "k":
                self.board_attributes |= B_CASTLE_KING
            elif fen[counter] == "q":
                self.board_attributes |= B_CASTLE_QUEEN
            counter += 1

        counter += 1
        if fen[counter] == '-':
            self.en_passant_target_square = 0
            counter += 2
        else:
            self.en_passant_target_square = algebraic_to_arraypos(fen[counter:counter+2])
            counter += 3

        numstr = ""
        while fen[counter] != " ":
            numstr += fen[counter]
            counter += 1
        self.halfmove_clock = int(numstr)

        counter += 1
        numstr = ""
        while counter < len(fen):
            numstr += fen[counter]
            counter += 1
        self.fullmove_number = int(numstr)
        self.initialize_piece_locations()

        if self.side_to_move_is_in_check():
            self.board_attributes |= BOARD_IN_CHECK
        else:
            self.board_attributes &= ~BOARD_IN_CHECK

        self.move_history = []
        self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)
        HISTORICAL_HASH_ARRAY[0] = self.cached_hash
        HISTORICAL_HASH_POSITION = 0

    def convert_to_fen(self, limited_fen=False):

        # limited_fen = only the board position and side to move, used when computing draw by repetition

        retval = ""
        for rank in range(90, 10, -10):
            num_blanks = 0
            for file in range(1, 9, 1):
                if self.board_array[rank + file] == EMPTY:
                    num_blanks += 1
                else:
                    if num_blanks > 0:
                        retval += str(num_blanks)
                        num_blanks = 0
                    retval += piece_to_string_dict[self.board_array[rank + file]]

            if num_blanks > 0:
                retval += str(num_blanks)
            if rank > 20:
                retval += "/"

        retval += " "

        if self.board_attributes & W_TO_MOVE:
            retval += "w"
        else:
            retval += "b"

        if not limited_fen:
            retval += " "

            castle_string = ""
            if self.board_attributes & W_CASTLE_KING:
                castle_string += "K"
            if self.board_attributes & W_CASTLE_QUEEN:
                castle_string += "Q"
            if self.board_attributes & B_CASTLE_KING:
                castle_string += "k"
            if self.board_attributes & B_CASTLE_QUEEN:
                castle_string += "q"
            if castle_string == "":
                castle_string = "-"
            retval += castle_string

            retval += " "

            if self.en_passant_target_square == 0:
                retval += "-"
            else:
                retval += arraypos_to_algebraic(self.en_passant_target_square)

            retval += " "

            retval += str(self.halfmove_clock) + " " + str(self.fullmove_number)

        return retval

    def print_move_history(self):
        outstr, halfmove = "", 0
        for move in self.move_history:
            halfmove += 1
            if halfmove % 2 == 1:
                outstr += ("%d. " % (1 + (halfmove // 2)))
            movestr = pretty_print_move(move[0])
            if halfmove % 2 == 0:
                outstr += movestr + "\n"
            else:
                numspaces = 10 - len(movestr)
                outstr += movestr + (numspaces * " ")
        return outstr

    cdef bint threefold_repetition(self):
        global HISTORICAL_HASH_POSITION, HISTORICAL_HASH_ARRAY, HISTORICAL_HASH_MIN_POSITION
        cdef:
            unsigned long long curhash
            int curcount, i

        # we test threefold_repettion as each move is applied, so we only need to look to see if the
        # current position is duplicated 2x previously.
        curhash = HISTORICAL_HASH_ARRAY[HISTORICAL_HASH_POSITION]
        curcount = 1
        for i in range(HISTORICAL_HASH_POSITION - 1, HISTORICAL_HASH_MIN_POSITION - 1, -1):
            if HISTORICAL_HASH_ARRAY[i] == curhash:
                curcount += 1
                if curcount >= 3:
                    return True
        return False

    cdef int evaluate_board(self):
        """

        :return: white score minus black score if white to move, otherwise the other way around.
        """
        global white_pawn_pst, white_knight_pst, white_bishop_pst, white_rook_pst, white_queen_pst
        global black_pawn_pst, black_knight_pst, black_bishop_pst, black_rook_pst, black_queen_pst
        global white_king_pst, white_king_endgame_pst, black_king_pst, black_king_endgame_pst

        cdef:
            int position_score, piece, square, phase, total_phase, wk_location, bk_location
            int white_majors, white_minors, black_majors, black_minors, white_pawn, black_pawn, key_square
            int early_game_white_king, late_game_white_king, early_game_black_king, late_game_black_king
            list locations
            double phase_pct, inv_phase_pct
            int wq, wr, wb, wn, wp, bq, br, bb, bn, bp, i

        # Performance - unroll this to eliminate using a python list
        wq = self.piece_count_locations[WQ][0]
        wr = self.piece_count_locations[WR][0]
        wb = self.piece_count_locations[WB][0]
        wn = self.piece_count_locations[WN][0]
        wp = self.piece_count_locations[WP][0]
        bq = self.piece_count_locations[BQ][0]
        br = self.piece_count_locations[BR][0]
        bb = self.piece_count_locations[BB][0]
        bn = self.piece_count_locations[BN][0]
        bp = self.piece_count_locations[BP][0]

        white_majors = wq + wr
        white_minors = wb + wn
        black_majors = bq + br
        black_minors = bb + bn


        if self.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            return 0  # Draw
        elif white_majors + white_minors + black_majors + black_minors + wp + bp == 0:
            return 0  # king vs. king = draw
        else:

            # tapered evaluation taken from https://chessprogramming.wikispaces.com/Tapered+Eval and modified

            position_score = 0
            # Another unrolled loop for cython purposes
            for i in range(1, self.piece_count_locations[WP][0] + 1):
                square = self.piece_count_locations[WP][i]
                position_score += white_pawn_pst[square]

            for i in range(1, self.piece_count_locations[WN][0] + 1):
                square = self.piece_count_locations[WN][i]
                position_score += white_knight_pst[square]

            for i in range(1, self.piece_count_locations[WB][0] + 1):
                square = self.piece_count_locations[WB][i]
                position_score += white_bishop_pst[square]

            for i in range(1, self.piece_count_locations[WR][0] + 1):
                square = self.piece_count_locations[WR][i]
                position_score += white_rook_pst[square]

            for i in range(1, self.piece_count_locations[WQ][0] + 1):
                square = self.piece_count_locations[WQ][i]
                position_score += white_queen_pst[square]

            for i in range(1, self.piece_count_locations[BP][0] + 1):
                square = self.piece_count_locations[BP][i]
                position_score += black_pawn_pst[square]

            for i in range(1, self.piece_count_locations[BN][0] + 1):
                square = self.piece_count_locations[BN][i]
                position_score += black_knight_pst[square]

            for i in range(1, self.piece_count_locations[BB][0] + 1):
                square = self.piece_count_locations[BB][i]
                position_score += black_bishop_pst[square]

            for i in range(1, self.piece_count_locations[BR][0] + 1):
                square = self.piece_count_locations[BR][i]
                position_score += black_rook_pst[square]

            for i in range(1, self.piece_count_locations[BQ][0] + 1):
                square = self.piece_count_locations[BQ][i]
                position_score += black_queen_pst[square]

            # Kings have a separate score based on time of game

            total_phase = 24
            phase = total_phase
            phase -= (bn + wn)
            phase -= (bb + wb)
            phase -= (2 * (br + wr))
            phase -= (4 * (bq + wq))

            # phase 0 = beginning
            # phase of total_phase = end
            # otherwise it is in the middle

            wk_location = self.piece_count_locations[WK][1]
            bk_location = self.piece_count_locations[BK][1]

            early_game_white_king = white_king_pst[wk_location]
            early_game_black_king = black_king_pst[bk_location]

            if phase == 0:
                position_score += early_game_white_king + early_game_black_king
            else:

                late_game_white_king = white_king_endgame_pst[wk_location]
                late_game_black_king = black_king_endgame_pst[bk_location]

                if black_majors + black_minors + white_majors + white_minors == 0:
                    # KP vs. KP optimization - white first
                    # This is an oversimplification for multiple reasons:
                    # 1) It doesn't penalize doubled-up pawns
                    # 2) It doesn't look to ensure that the pawn has no opposing pawn in the way (e.g. it doesn't
                    #    ensure that it's a passed pawn.
                    # In the first case, it's hard for a king to be in the key square of 2 pawns, and in the second
                    #   if we get the king to the key square, then the king will be able to capture any pawn that is
                    #   in the way.
                    for i in range(1, self.piece_count_locations[WP][0] + 1):
                        white_pawn = self.piece_count_locations[WP][i]
                        for key_square in white_kpk_key_squares[white_pawn]:
                            if self.board_array[key_square] == WK:
                                late_game_white_king += 75
                                break
                    for i in range(1, self.piece_count_locations[BP][0] + 1):
                        black_pawn = self.piece_count_locations[BP][i]
                        for key_square in black_kpk_key_squares[black_pawn]:
                            if self.board_array[key_square] == BK:
                                late_game_black_king += 75
                                break

                    position_score += late_game_white_king + late_game_black_king  # phase = 0, no need to do the phase math
                else:
                    # add phase/total * late + (1- (phase/total)) * early

                    phase_pct = phase/total_phase
                    inv_phase_pct = 1-phase_pct

                    position_score += int(inv_phase_pct * (early_game_white_king + early_game_black_king))
                    position_score += int(phase_pct * (late_game_white_king + late_game_black_king))

            if not (self.board_attributes & W_TO_MOVE):
                position_score = position_score * -1
            return position_score


    cdef unapply_move(self):

        global HISTORICAL_HASH_POSITION, HISTORICAL_HASH_ARRAY

        cdef:
            Move move
            int ep_target, halfmove_clock, fullmove_number
            unsigned int attrs
            str cached_fen
            unsigned long long cached_hash
            int start, end, piece_moved, piece_captured, promoted_to, move_flags

        move, attrs, ep_target, halfmove_clock, fullmove_number, cached_fen, cached_hash = self.move_history.pop()

        start = move & START
        end = ((move & END) >> END_SHIFT)
        piece_moved = ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT)
        piece_captured = ((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT)
        promoted_to = ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT)
        move_flags = ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT)

        # move piece back
        if promoted_to:
            if promoted_to & BLACK:
                self.board_array[start] = BP
                self.add_piece_to_location_array(BP, start)
            else:
                self.board_array[start] = WP
                self.add_piece_to_location_array(WP, start)
            self.remove_piece_from_location_array(promoted_to, end)

        else:
            self.board_array[start] = piece_moved
            self.remove_piece_from_location_array(piece_moved, end)
            self.add_piece_to_location_array(piece_moved, start)

        if piece_captured:
            # if it was a capture, replace captured piece
            if move_flags & MOVE_EN_PASSANT:
                self.en_passant_target_square = end
                self.board_array[end] = EMPTY
                if piece_captured == BP:
                    pdest = end-10
                    self.board_array[pdest] = BP
                    self.add_piece_to_location_array(BP, pdest)
                else:
                    pdest = end+10
                    self.board_array[pdest] = WP
                    self.add_piece_to_location_array(WP, pdest)
            else:
                self.board_array[end] = piece_captured
                self.add_piece_to_location_array(piece_captured, end)
        else:
            self.board_array[end] = EMPTY

            if move_flags & MOVE_CASTLE:
                # need to move the rook back too
                if end == 27:  # white, king side
                    self.add_piece_to_location_array(WR, 28)
                    self.board_array[28] = WR
                    self.remove_piece_from_location_array(WR, 26)
                    self.board_array[26] = EMPTY
                elif end == 23:  # white, queen side
                    self.add_piece_to_location_array(WR, 21)
                    self.board_array[21] = WR
                    self.remove_piece_from_location_array(WR, 24)
                    self.board_array[24] = EMPTY
                elif end == 97:  # black, king side
                    self.add_piece_to_location_array(BR, 98)
                    self.board_array[98] = BR
                    self.remove_piece_from_location_array(BR, 96)
                    self.board_array[96] = EMPTY
                elif end == 93:  # black, queen side
                    self.add_piece_to_location_array(BR, 91)
                    self.board_array[91] = BR
                    self.remove_piece_from_location_array(BR, 94)
                    self.board_array[94] = EMPTY

        # reset settings
        self.board_attributes = attrs
        self.halfmove_clock = halfmove_clock
        self.fullmove_number = fullmove_number
        self.en_passant_target_square = ep_target
        self.cached_fen = cached_fen
        self.cached_hash = cached_hash

        HISTORICAL_HASH_ARRAY[HISTORICAL_HASH_POSITION] = 0
        HISTORICAL_HASH_POSITION -= 1


    cdef apply_move(self, Move move):
        global TRANSPOSITION_TABLE, HISTORICAL_HASH_ARRAY, HISTORICAL_HASH_POSITION

        # this function doesn't validate that the move is legal, just applies the move
        # the asserts are mostly for debugging, may want to remove for performance later.

        cdef:
            int start, end, piece_moving, piece_captured, capture_diff, promoted_to, move_flags, ppos
            unsigned int oldattrs, attrdiffs



        self.move_history.append((move, self.board_attributes, self.en_passant_target_square,
                                  self.halfmove_clock, self.fullmove_number, self.cached_fen, self.cached_hash))


        start = move & START
        end = ((move & END) >> END_SHIFT)
        piece_moving = ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT)
        piece_captured = ((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT)
        # not used in this function -
        # capture_diff = ((move & CAPTURE_DIFFERENTIAL) >> CAPTURE_DIFFERENTIAL_SHIFT) - CAPTURE_DIFFERENTIAL_OFFSET
        promoted_to = ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT)
        move_flags = ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT)

        self.remove_piece_from_location_array(piece_moving, start)
        self.add_piece_to_location_array(piece_moving, end)

        self.board_array[end] = piece_moving
        self.board_array[start] = EMPTY

        # This is a big unroll for Cython performance
        if piece_moving == WP:
            self.cached_hash ^= TRANSPOSITION_TABLE.whitep[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.whitep[end]
        elif piece_moving == BP:
            self.cached_hash ^= TRANSPOSITION_TABLE.blackp[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.blackp[end]
        elif piece_moving == WK:
            self.cached_hash ^= TRANSPOSITION_TABLE.whitek[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.whitek[end]
        elif piece_moving == BK:
            self.cached_hash ^= TRANSPOSITION_TABLE.blackk[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.blackk[end]
        elif piece_moving == WN:
            self.cached_hash ^= TRANSPOSITION_TABLE.whiten[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.whiten[end]
        elif piece_moving == BN:
            self.cached_hash ^= TRANSPOSITION_TABLE.blackn[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.blackn[end]
        elif piece_moving == WB:
            self.cached_hash ^= TRANSPOSITION_TABLE.whiteb[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.whiteb[end]
        elif piece_moving == BB:
            self.cached_hash ^= TRANSPOSITION_TABLE.blackb[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.blackb[end]
        elif piece_moving == WR:
            self.cached_hash ^= TRANSPOSITION_TABLE.whiter[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.whiter[end]
        elif piece_moving == BR:
            self.cached_hash ^= TRANSPOSITION_TABLE.blackr[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.blackr[end]
        elif piece_moving == WQ:
            self.cached_hash ^= TRANSPOSITION_TABLE.whiteq[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.whiteq[end]
        elif piece_moving == BQ:
            self.cached_hash ^= TRANSPOSITION_TABLE.blackq[start]
            self.cached_hash ^= TRANSPOSITION_TABLE.blackq[end]
        else:
            raise ValueError("Invalid Piece {:d}".format(piece_moving))

        # Remove captured pawn if en passant capture
        if piece_captured:
            if move_flags & MOVE_EN_PASSANT:
                if piece_moving & BLACK:
                    ppos = end+10
                    # black is moving, blank out the space 10 more than destination space
                    self.remove_piece_from_location_array(WP, ppos)
                    self.cached_hash ^= TRANSPOSITION_TABLE.whitep[ppos]
                    self.board_array[ppos] = EMPTY
                else:
                    ppos = end-10
                    self.remove_piece_from_location_array(BP, ppos)
                    self.cached_hash ^= TRANSPOSITION_TABLE.blackp[ppos]
                    self.board_array[ppos] = EMPTY
            else:
                try:
                    self.remove_piece_from_location_array(piece_captured, end)
                    self.cached_hash ^= <unsigned long long>(TRANSPOSITION_TABLE.board_mask_dict[piece_captured][end])
                except:
                    print(self.print_move_history())
                    print(self.pretty_print(True))
                    raise

        # Reset en_passant_target_square and set below if it needs to be
        if self.en_passant_target_square:
            self.cached_hash ^= TRANSPOSITION_TABLE.enpassanttarget[self.en_passant_target_square]
            self.en_passant_target_square = 0


        if move_flags & MOVE_CASTLE:
            # the move includes the king, need to move the rook
            if end == 27:  # white, king side
                # assert self.white_can_castle_king_side
                self.remove_piece_from_location_array(WR, 28)
                self.board_array[28] = EMPTY
                self.add_piece_to_location_array(WR, 26)
                self.board_array[26] = WR
                self.cached_hash ^= TRANSPOSITION_TABLE.whitecastleking
                if self.board_attributes & W_CASTLE_QUEEN:
                    self.cached_hash ^= TRANSPOSITION_TABLE.whitecastlequeen
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.whiter[28]
                self.cached_hash ^= TRANSPOSITION_TABLE.whiter[26]
            elif end == 23:  # white, queen side
                # assert self.white_can_castle_queen_side
                self.remove_piece_from_location_array(WR, 21)
                self.board_array[21] = EMPTY
                self.add_piece_to_location_array(WR, 24)
                self.board_array[24] = WR
                self.cached_hash ^= TRANSPOSITION_TABLE.whitecastlequeen
                if self.board_attributes & W_CASTLE_KING:
                    self.cached_hash ^= TRANSPOSITION_TABLE.whitecastleking
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.whiter[21]
                self.cached_hash ^= TRANSPOSITION_TABLE.whiter[24]
            elif end == 97:  # black, king side
                # assert self.black_can_castle_king_side
                self.remove_piece_from_location_array(BR, 98)
                self.board_array[98] = EMPTY
                self.add_piece_to_location_array(BR, 96)
                self.board_array[96] = BR
                self.cached_hash ^= TRANSPOSITION_TABLE.blackcastleking
                if self.board_attributes & B_CASTLE_QUEEN:
                    self.cached_hash ^= TRANSPOSITION_TABLE.blackcastlequeen
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.blackr[98]
                self.cached_hash ^= TRANSPOSITION_TABLE.blackr[96]
            elif end == 93:  # black, queen side
                # assert self.black_can_castle_queen_side
                self.remove_piece_from_location_array(BR, 91)
                self.board_array[91] = EMPTY
                self.add_piece_to_location_array(BR, 94)
                self.board_array[94] = BR
                self.cached_hash ^= TRANSPOSITION_TABLE.blackcastlequeen
                if self.board_attributes & B_CASTLE_KING:
                    self.cached_hash ^= TRANSPOSITION_TABLE.blackcastleking
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.blackr[91]
                self.cached_hash ^= TRANSPOSITION_TABLE.blackr[94]
            else:
                raise ValueError("Invalid Castle Move ", start, end)
        elif promoted_to:
            self.remove_piece_from_location_array(piece_moving, end)
            self.cached_hash ^= <unsigned long long>(TRANSPOSITION_TABLE.board_mask_dict[piece_moving][end])
            self.board_array[end] = promoted_to
            self.cached_hash ^= <unsigned long long>(TRANSPOSITION_TABLE.board_mask_dict[promoted_to][end])
            self.add_piece_to_location_array(promoted_to, end)

        elif move_flags & MOVE_DOUBLE_PAWN:
            if piece_moving == WP:
                self.en_passant_target_square = end - 10
            else:
                self.en_passant_target_square = end + 10
            self.cached_hash ^= TRANSPOSITION_TABLE.enpassanttarget[self.en_passant_target_square]

        oldattrs = self.board_attributes
        # other conditions to end castling - could make this more efficient
        if self.board_attributes & (W_CASTLE_KING | W_CASTLE_QUEEN):
            if piece_moving == WK:
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
            elif piece_moving == WR:
                if start == 21:
                    self.board_attributes &= ~W_CASTLE_QUEEN
                elif start == 28:
                    self.board_attributes &= ~W_CASTLE_KING
        if self.board_attributes & (B_CASTLE_KING | B_CASTLE_QUEEN):
            if piece_moving == BK:
                self.board_attributes &= ~(B_CASTLE_KING | B_CASTLE_QUEEN)
            elif piece_moving == BR:
                if start == 91:
                    self.board_attributes &= ~B_CASTLE_QUEEN
                if start == 98:
                    self.board_attributes &= ~B_CASTLE_KING

        if move_flags & MOVE_CHECK:
            self.board_attributes |= BOARD_IN_CHECK
        else:
            self.board_attributes &= ~BOARD_IN_CHECK

        attrdiffs = oldattrs ^ self.board_attributes
        if attrdiffs:
            if attrdiffs & W_CASTLE_KING:
                self.cached_hash ^= TRANSPOSITION_TABLE.whitecastleking
            if attrdiffs & W_CASTLE_QUEEN:
                self.cached_hash ^= TRANSPOSITION_TABLE.whitecastlequeen
            if attrdiffs & B_CASTLE_QUEEN:
                self.cached_hash ^= TRANSPOSITION_TABLE.blackcastlequeen
            if attrdiffs & B_CASTLE_KING:
                self.cached_hash ^= TRANSPOSITION_TABLE.blackcastleking


        if (piece_moving & PAWN) or piece_captured:
            self.halfmove_clock = 0
        else:
            self.halfmove_clock += 1

        if not self.board_attributes & W_TO_MOVE:
            self.fullmove_number += 1

        self.board_attributes ^= W_TO_MOVE
        self.cached_hash ^= TRANSPOSITION_TABLE.whitetomove
        self.cached_hash ^= TRANSPOSITION_TABLE.blacktomove

        HISTORICAL_HASH_POSITION += 1
        HISTORICAL_HASH_ARRAY[HISTORICAL_HASH_POSITION] = self.cached_hash


    cdef bint side_to_move_is_in_check(self):

        global WHITE_CHECK_TABLE, BLACK_CHECK_TABLE

        cdef:
            int curpos, occupant, kingpos, cur_attack, square_to_check


        if self.board_attributes & W_TO_MOVE:
            # The white and black loops are the same except for the test for is piece of the enemy color.
            # By separating this I can save one comparison per loop.  There may be a bitwise way to do this,
            # by xoring just the color bit with the color that is moving, but for now I will deal with the
            # extra long code.

            kingpos = self.piece_count_locations[WK][1]
            curpos = 0
            square_to_check = WHITE_CHECK_TABLE[kingpos][curpos][0]
            while square_to_check != 0:
                occupant = self.board_array[square_to_check]
                if occupant & WHITE_CHECK_TABLE[kingpos][curpos][1]:
                    # is the piece of the enemy color?
                    if occupant & BLACK:
                        return True  # bust out of this loop
                    else:
                        curpos = WHITE_CHECK_TABLE[kingpos][curpos][2]   # this direction is blocked
                elif occupant:
                    curpos = WHITE_CHECK_TABLE[kingpos][curpos][2]
                else:
                    curpos += 1
                square_to_check = WHITE_CHECK_TABLE[kingpos][curpos][0]
        else:
            kingpos = self.piece_count_locations[BK][1]
            curpos = 0
            square_to_check = BLACK_CHECK_TABLE[kingpos][curpos][0]
            while square_to_check != 0:
                occupant = self.board_array[square_to_check]
                if occupant & BLACK_CHECK_TABLE[kingpos][curpos][1]:
                    # is the piece of the enemy color?
                    if not (occupant & BLACK):
                        return True  # bust out of this loop
                    else:
                        curpos = BLACK_CHECK_TABLE[kingpos][curpos][2]
                elif occupant:
                    curpos = BLACK_CHECK_TABLE[kingpos][curpos][2]
                else:
                    curpos += 1
                square_to_check = BLACK_CHECK_TABLE[kingpos][curpos][0]

        return False

    cdef list generate_pinned_list(self, bint for_defense = True):
        # If we are looking at defense, we are generating the list of pieces that are pinned.
        # that is, a friendly piece blocking an enemy attacker from a friendly king.
        # If we are looking for attack, we are generating the list of pieces which, if moved,
        # would lead to discovered checks.  So this is a friendly piece, blocking a friendly attacker from the
        # enemy king.

        cdef:
            list retlist = []
            int blocking_piece_color, defending_king, attacking_piece_color, king_position
            int attacking_bishop, attacking_rook, attacking_queen, velocity, cur_pos, cur_piece, pinning_pos
            bint test_sliders, test_diagonals


        # if white to move and for defense
        #    white king, blocked by white pieces, attacked by black piece
        # if white to move and for offense
        #    black king, blocked by white pieces, attacked by white pieces
        # if black to move and for defense
        #    black king, blocked by black pieces, attacked by white pieces
        # if black to move and for offense
        #    white king, blocked by black pieces, attacked by black pieces

        if self.board_attributes & W_TO_MOVE:
            blocking_piece_color = WHITE
            if for_defense:
                defending_king = WK
                attacking_piece_color = BLACK
            else:
                defending_king = BK
                attacking_piece_color = WHITE

        else:
            blocking_piece_color = BLACK
            if for_defense:
                defending_king = BK
                attacking_piece_color = WHITE
            else:
                defending_king = WK
                attacking_piece_color = BLACK

        king_position = self.piece_count_locations[defending_king][1]
        attacking_bishop = BISHOP | attacking_piece_color
        attacking_rook = ROOK | attacking_piece_color
        attacking_queen = QUEEN | attacking_piece_color


        test_sliders = False
        test_diagonals = False
        if self.piece_count_locations[attacking_queen][0] > 0:
            test_sliders = True
            test_diagonals = True
        else:
            if self.piece_count_locations[attacking_bishop][0] > 0:
                test_diagonals = True
            if self.piece_count_locations[attacking_rook][0] > 0:
                test_sliders = True

        if test_diagonals:
            for velocity in [-9, -11, 9, 11]:
                cur_pos = king_position + velocity
                cur_piece = self.board_array[cur_pos]
                while cur_piece == EMPTY:
                    cur_pos += velocity
                    cur_piece = self.board_array[cur_pos]
                if (cur_piece & BLACK) == blocking_piece_color and cur_piece & OFF_BOARD == 0:
                    pinning_pos = cur_pos + velocity
                    while self.board_array[pinning_pos] == EMPTY:
                        pinning_pos += velocity
                    if self.board_array[pinning_pos] in [attacking_queen, attacking_bishop]:
                        retlist.append(cur_pos)
        if test_sliders:
            for velocity in [-10, -1, 1, 10]:
                cur_pos = king_position + velocity
                cur_piece = self.board_array[cur_pos]
                while self.board_array[cur_pos] == EMPTY:
                    cur_pos += velocity
                    cur_piece = self.board_array[cur_pos]
                if (cur_piece & BLACK) == blocking_piece_color and cur_piece & OFF_BOARD == 0:
                    # if the piece is of the opposite color than the enemy queen, it's a friendly piece
                    pinning_pos = cur_pos + velocity
                    while self.board_array[pinning_pos] == EMPTY:
                        pinning_pos += velocity
                    if self.board_array[pinning_pos] in [attacking_queen, attacking_rook]:
                        retlist.append(cur_pos)

        return retlist


    cdef required_post_move_updates(self):
        # These are required updates after the real move is made.  These updates are not needed during move
        # evaluation, so are left out of apply/unapply move for performance reasons.
        global HISTORICAL_HASH_MIN_POSITION, HISTORICAL_HASH_POSITION

        self.cached_fen = self.convert_to_fen(True)
        if self.halfmove_clock == 0:
            HISTORICAL_HASH_MIN_POSITION = HISTORICAL_HASH_POSITION



cdef print_computer_thoughts(int orig_search_depth, int score, list movelist):
    global START_TIME, DEBUG, DEBUGFILE

    cdef:
        datetime.timedelta delta
        datetime.datetime curtime
        str movestr
        long centiseconds
        str outstr

    movestr = ""
    curtime = datetime.now()
    delta = curtime - START_TIME
    centiseconds = (100 * delta.seconds) + (delta.microseconds // 10000)
    for move in movelist:
        movestr += pretty_print_move(move) + " "
    outstr = str(orig_search_depth) + " " + str(score) + " " + str(centiseconds) + " " + str(NODES) + " " + movestr

    if DEBUG:
        DEBUGFILE.write(outstr + "\n")
    print(outstr)


cdef tuple negamax_recurse(ChessBoard board, int depth, long alpha, long beta, int depth_at_root,
                            list best_known_line = []):

    global NODES, DEBUG, POST, TRANSPOSITION_TABLE, KILLER_MOVELIST

    cdef:
        long original_alpha, cache_score, best_score, score
        int cache_depth, cache_score_type, mate_in_one_score
        tuple cached_position, tmptuple
        list cached_ml, cached_opponent_movelist, move_list, best_move_sequence, move_sequence
        ChessMoveListGenerator move_list_generator
        Move my_best_move, move, previous_best_move
        bint found_next_move_in_line = False

    # Pseudocode can be found at: https://en.wikipedia.org/wiki/Negamax

    # TO-DO:  Insert "if we have exceeded our maximum time, return, setting some flag that we quit the search.
    # Even though we won't be able to finish the ply, we will have built out the transposition cache.

    NODES += 1
    original_alpha = alpha

    if board.threefold_repetition():
        return 0, [NULL_MOVE]  # Draw - stop searching this position.  Do not cache, as transposition may not always be draw

    cached_position = TRANSPOSITION_TABLE.probe(board)
    if cached_position is not None:
        cached_ml, cache_depth, cache_score, cache_score_type, cached_opponent_movelist = cached_position

        if cache_depth >= depth:
            if cache_score_type == CACHE_SCORE_EXACT:
                # Noted we spent times searching for "better" mates, because a forced mate gave a cached score
                # based on the depth of the stored transposition.  Needs to be reduced in this path.
                #
                # Example - We are doing 12-ply search.  At our depth of 6, we hit a node with depth 10 in the
                # TT table, with score 100002.  That meant that at depth 2 there was mate - so it was mate in 4
                # (8 plies) in the TT table.  If we use that, we don't want to give it the same score of 100002, because
                # from here, that same mate is at a depth of -2 - score schould be 99998.  We need to reduce the score
                # by (cached_depth - depth)

                if cache_score <= -100000:  # losing mate
                    cache_score += (cache_depth - depth)
                elif cache_score >= 100000:
                    cache_score -= (cache_depth - depth)
                return cache_score, cached_opponent_movelist
            elif cache_score_type == CACHE_SCORE_LOW:
                if cache_score > alpha:
                    alpha = cache_score
                    best_known_line = cached_opponent_movelist
            elif cache_score_type == CACHE_SCORE_HIGH:
                if cache_score < beta:
                    beta = cache_score
                    best_known_line = cached_opponent_movelist
            if alpha >= beta:
                return cache_score, cached_opponent_movelist

        move_list = cached_ml
        if len(best_known_line) > 0:
            previous_best_move = best_known_line[0]
            for i in range(len(move_list)):
                if move_list[i] == previous_best_move:
                    move_list = [move_list[i]] + move_list[0:i] + move_list[i+1:]
                    found_next_move_in_line = True
                    break
            if not found_next_move_in_line:
                best_known_line = []  # line isn't valid from here down

    else:
        if len(best_known_line) > 0:
            previous_best_move = best_known_line[0]
        else:
            previous_best_move = NULL_MOVE

        killer_move1 = KILLER_MOVELIST[2*depth]
        killer_move2 = KILLER_MOVELIST[(2*depth)+1]

        move_list_generator = ChessMoveListGenerator(board)
        move_list_generator.generate_move_list(previous_best_move , killer_move1, killer_move2)
        move_list = move_list_generator.move_list


    if len(move_list) == 0:
        if board.board_attributes & BOARD_IN_CHECK:
            retval = (-100000 - depth)
        else:
            retval = 0
        return retval, [NULL_MOVE]

    if depth <= 0:
        # To-Do: Quiescence.  I had a quiescence search here but it led to weird results.
        # Basic theory of Quiescence is to take the move list, and reduce it significantly and consider
        # only those moves that would occur if the curent state is not stable.

        # For now - just return static evaluation
        return board.evaluate_board(), []

    best_score = -101000
    my_best_move = NULL_MOVE
    mate_in_one_score = 100000 + (depth-1)
    best_move_sequence = []
    for move in move_list:
        board.apply_move(move)

        # a litle hacky, but cannot use unpacking while also multiplying the score portion by -1
        tmptuple = (negamax_recurse(board, depth-1, -1 * beta, -1 * alpha, depth_at_root, best_known_line[1:]))
        score = -1 * tmptuple[0]
        move_sequence = tmptuple[1]

        board.unapply_move()

        if score > best_score:
            best_score = score
            best_move_sequence = move_sequence
            my_best_move = move
        if score > alpha:
            alpha = score
            if depth == depth_at_root and POST:
                print_computer_thoughts(depth, best_score, [my_best_move] + best_move_sequence)
        if alpha >= beta:
            if not move & PIECE_CAPTURED:
                KILLER_MOVELIST[(2*depth+1)] = KILLER_MOVELIST[2*depth]
                KILLER_MOVELIST[2*depth] = move
            break  # alpha beta cutoff
        if alpha >= mate_in_one_score:
            # will not do any better
            break

    if best_score <= original_alpha:
        cache_score_type = CACHE_SCORE_HIGH
    elif best_score >= beta:
        cache_score_type = CACHE_SCORE_LOW
    else:
        cache_score_type = CACHE_SCORE_EXACT

    TRANSPOSITION_TABLE.insert(board, (move_list, depth, best_score, cache_score_type,
                                                           [my_best_move] + best_move_sequence))

    return best_score, [my_best_move] + best_move_sequence

cdef tuple process_computer_move(ChessBoard board, list best_known_line, int search_depth=4, long search_time=10000):
    global START_TIME, XBOARD, KILLER_MOVELIST
    global NODES

    cdef:
        long best_score
        datetime.timedelta delta
        datetime.datetime end_time
        Move computer_move
        str movestr, movetext

    START_TIME = datetime.now()
    NODES = 0
    KILLER_MOVELIST = [NULL_MOVE] * (2 * (search_depth + 1))

    best_score, best_known_line = negamax_recurse(board, search_depth, -101000, 101000, search_depth, best_known_line)

    assert(len(best_known_line) > 0)

    end_time = datetime.now()
    computer_move = best_known_line[0]

    if not XBOARD:
        print("Time now: {:%Y-%m-%d %H:%M:%S}".format(end_time))
        print("Elapsed time: " + str(end_time - START_TIME))
        print ("Nodes Searched: {:d} - Nodes per second {:.2f}\n".format(NODES, NODES / (end_time-START_TIME).total_seconds()))
        print("Move made: %s : Score = %d" % (pretty_print_move(computer_move, True), best_score))
        movestr = ""
        for c in best_known_line:
            movestr += pretty_print_move(c) + " "
        print(movestr)
    else:
        movetext = arraypos_to_algebraic(computer_move & START)
        movetext += arraypos_to_algebraic((computer_move & END) >> END_SHIFT)
        if computer_move & PROMOTED_TO:
            movetext += piece_to_string_dict[(computer_move & PROMOTED_TO) >> PROMOTED_TO_SHIFT].lower()
        printcommand("move " + movetext)

    board.apply_move(computer_move)

    # We use the best_known_line to order moves in the next search if the opponent takes the next move in the line
    return best_score, best_known_line[1:]


# Required to handle these signals if you want to use xboard

def handle_sigint(signum, frame):
    global DEBUGFILE
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGINT at " + str(datetime.now()) + "\n")
    sys.exit()


def handle_sigterm(signum, frame):
    global DEBUGFILE
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGTERM at " + str(datetime.now()) + "\n")
    sys.exit()


cdef bint test_for_end(ChessBoard board):

    cdef:
        ChessMoveListGenerator move_list

    # Test for draw by repetition:
    if board.threefold_repetition():
        printcommand("1/2-1/2 {Stalemate - Repetition}")

    move_list = ChessMoveListGenerator(board)
    move_list.generate_move_list()
    if len(move_list.move_list) == 0:
        # either it's a checkmate or a stalemate
        if board.side_to_move_is_in_check():
            if board.board_attributes & W_TO_MOVE:
                printcommand("0-1 {Black mates}")
            else:
                printcommand("1-0 {White mates}")
        else:
            printcommand("1/2-1/2 {Stalemate}")
        return True
    elif (board.piece_count_locations[WP][0] + board.piece_count_locations[WB][0] + board.piece_count_locations[WN][0] + board.piece_count_locations[WR][0] +
                board.piece_count_locations[WQ][0] == 0) and (board.piece_count_locations[BP][0] + board.piece_count_locations[BB][0] +
                board.piece_count_locations[BN][0] + board.piece_count_locations[BR][0] + board.piece_count_locations[BQ][0] == 0):
        # Note: Per xboard documentation, you can do stalemate with KK, KNK, KBK, or KBKB with both B's on same
        # color.  For now, only doing KK.
        printcommand("1/2-1/2 {Stalemate - insufficient material}")
        return True  # king vs. king = draw
    else:
        return False


cdef print_supported_commands():

    print("Sample move syntax:")
    print("     e2e4  - regular move")
    print("     a7a8q - promotion")
    print("     e1g1  - castle")
    print("")
    print("Other commands:")
    print("")
    print("     both          - computer plays both sides - cannot break until end of game")
    print("     debug         - enable debugging output / chessdebug.txt log file")
    print("     draw          - request draw due to 50 move rule")
    print("     force         - human plays both white and black")
    print("     go            - computer takes over for color currently on move")
    print("                   - NOTE: engine will pause between moves if you make computer play both sides")
    print("     help          - this list")
    print("     history       - print the game's move history")
    print("     new           - begin new game, computer black")
    print("     nopost        - disable POST")
    print("     ping TEXT     - reply with 'pong TEXT'")
    print("     post          - see details on Bejola's thinking")
    print("                   - format: PLY SCORE TIME NODES MOVE_TREE")
    print("                   - where TIME is in centiseconds, and NODES is nodes searched. SCORE < 0 favors black")
    print("     print         - print the board to the terminal")
    print("     printpos      - print a list of pieces and their current positions")
    print("     quit          - exit game")
    print("     remove        - go back a full move")
    print("     resign        - resign your position")
    print("     sd DEPTH      - set search depth to DEPTH plies.  Over 6 will be very slow.  Machine will search")
    print("                   - this depth regardless of time needed.  Default is 4.")
    #  print("     st TIME       - set search time to TIME seconds per move.  Currently, this is a target and is not")
    #  print("                   - strictly enforced, but will be close.  Machine will exceed DEPTH if time remains.")
    #  print("                   - Default is 10 seconds.")
    print("     setboard FEN  - set current position to the FEN that is specified")
    print("     undo          - go back a half move (better: use 'remove' instead)")
    print("     xboard        - use xboard (GNU Chess) protocol")
    print("                   - this command is automatically sent by xboard. Should only")
    print("                   - be used interactively if you want to debug xboard issues.")


cdef printcommand(str command):
    global DEBUG, XBOARD, DEBUGFILE
    print(command)
    if DEBUG and XBOARD and DEBUGFILE is not None:
        DEBUGFILE.write("Command sent: " + command + "\n")
        DEBUGFILE.flush()

cdef tuple convert_score_to_mate_in_x(ChessBoard board, long score, int previous_search_depth, int original_search_depth):
    cdef:
        int white_mate_in_plies = -1
        int black_mate_in_plies = -1
        int mate_in

    if score >= 99990:
        mate_in = previous_search_depth - (score-100000)  # this is in plies
        if board.board_attributes & W_TO_MOVE:
            # black made the last move
            black_mate_in_plies = min(original_search_depth, mate_in)
        else:
            white_mate_in_plies = min(original_search_depth, mate_in)
    elif score <= -99990:
        mate_in = previous_search_depth - (abs(score)-100000)
        if board.board_attributes & W_TO_MOVE:
            white_mate_in_plies = min(original_search_depth, mate_in)
        else:
            black_mate_in_plies = min(original_search_depth, mate_in)

    return white_mate_in_plies, black_mate_in_plies

cdef int effective_late_game_search_depth(ChessBoard board, int search_depth, int white_mate_in_plies, int black_mate_in_plies):
    if board.board_attributes & W_TO_MOVE:
        if white_mate_in_plies >= 1:
            return white_mate_in_plies
        elif black_mate_in_plies >= 1:
            return min([search_depth, black_mate_in_plies + 2])  # give black 2 extra plies to search to avoid mate
        else:
            return search_depth
    else:
        if black_mate_in_plies >= 1:
            return black_mate_in_plies
        elif white_mate_in_plies >= 1:
            return min([search_depth, white_mate_in_plies + 2])
        else:
            return search_depth

cpdef play_game(str debugfen=""):

    cdef:
        ChessBoard b
        bint computer_is_black, computer_is_white, done_with_current_game
        int search_depth, piece, loc
        long search_time
        Move human_move
        str command, tmpstr
        list best_known_line = []

        # Since I'm doing a bunch of really deep searches in late-game puzzles, I want the search depth to decrease
        # if a mate is being forced.  However, the side losing should be able to search deeper.
        int w_mate_in_plies = -1
        int b_mate_in_plies = -1


    global DEBUG, XBOARD, POST, NODES, DEBUGFILE, TRANSPOSITION_TABLE
    # xboard integration requires us to handle these two signals
    signal.signal(signal.SIGINT, handle_sigint)
    signal.signal(signal.SIGTERM, handle_sigterm)

    # Acknowledgement: Thanks to Sam Tannous - I relied heavily on his implementation of xboard integration
    # in his "shatranj" engine - https://github.com/stannous/shatranj - only putting in xboard because my kids
    # insisted :).

    parser = argparse.ArgumentParser(description="Play chess with Bejola!")
    parser.add_argument("--debug", help="print debug messages during play", action="store_true", default=False)
    args = parser.parse_args()
    DEBUG = args.debug
    XBOARD = False
    POST = False
    NODES = 0

    if DEBUG:
        DEBUGFILE = open("chessdebug.txt", "w")
    else:
        DEBUGFILE = None

    b = ChessBoard()
    if debugfen != "":
        b.load_from_fen(debugfen)
    else:
        b.initialize_start_position()
    computer_is_black = True
    computer_is_white = False
    search_depth = 4
    effective_depth = 4
    search_time = 10000  # milliseconds

    expected_opponent_move = NULL_MOVE
    counter_to_expected_opp_move = NULL_MOVE

    done_with_current_game = False

    while True:
        # Check for mate/stalemate
        done_with_current_game = test_for_end(b)
        if done_with_current_game:
            computer_is_black = False
            computer_is_white = False
        elif not XBOARD:
            if b.side_to_move_is_in_check():
                print("Check!")
            if b.board_attributes & W_TO_MOVE and w_mate_in_plies >= 1:
                print("White to mate in {:d}".format(w_mate_in_plies // 2))
            elif not (b.board_attributes & W_TO_MOVE) and b_mate_in_plies >= 1:
                print("Black to mate in {:d}".format(b_mate_in_plies // 2))


        if ((b.board_attributes & W_TO_MOVE) and computer_is_white) or \
                            ((not (b.board_attributes & W_TO_MOVE)) and computer_is_black):
            effective_depth = effective_late_game_search_depth(b, search_depth, w_mate_in_plies, b_mate_in_plies)

            print ("DEBUG: Effective Depth = {:d}".format(effective_depth))
            best_score, best_known_line = process_computer_move(b, best_known_line, effective_depth)
            b.required_post_move_updates()
            w_mate_in_plies, b_mate_in_plies = convert_score_to_mate_in_x(b, best_score, effective_depth, search_depth)
            if not XBOARD:
                    print(b.pretty_print(True))
                    print(b.print_move_history())
        else:
            if DEBUG and XBOARD:
                DEBUGFILE.write("Waiting for command - " + str(datetime.now()) + "\n")
            command = input()
            if DEBUG and XBOARD:
                DEBUGFILE.write("Command received: " + command + "\n")
                DEBUGFILE.flush()

            # xboard documentation can be found at http://home.hccnet.nl/h.g.muller/engine-intf.html
            if command == "xboard" or command[0:8] == "protover":
                XBOARD = True
                printcommand('feature myname="Bejola0.8"')
                printcommand("feature ping=1")
                printcommand("feature setboard=1")
                printcommand("feature san=0")
                printcommand("feature sigint=0")
                printcommand("feature sigterm=1")
                printcommand("feature reuse=1")
                printcommand("feature time=0")
                printcommand("feature usermove=0")
                printcommand("feature colors=1")
                printcommand("feature nps=0")
                printcommand("feature debug=1")
                printcommand("feature analyze=0")
                printcommand("feature done=1")
            elif command[0:4] == "ping":
                printcommand("pong " + command[5:])
            elif command == "quit":
                sys.exit()
            elif command == "new":
                b.initialize_start_position()
                computer_is_black = True
                computer_is_white = False
                done_with_current_game = False
            elif command == "debug":
                # This command is only entered by humans, not Xboard.  Xboard must set debug via the command-line flag.
                # This command toggles debug mode on and off.
                if not DEBUG:
                    DEBUG = True
                    if DEBUGFILE is None:
                        DEBUGFILE = open("chessdebug.txt", "w")
                else:
                    DEBUG = False
                    DEBUGFILE.close()
                    DEBUGFILE = None
            elif command == "force":
                computer_is_black = False
                computer_is_white = False
                best_known_line = []
            elif command == "both":
                    computer_is_white = True
                    computer_is_black = True
            elif command == "go":
                if b.board_attributes & W_TO_MOVE:
                    computer_is_white = True
                    computer_is_black = False
                else:
                    computer_is_black = True
                    computer_is_white = False
            elif command[0:8] == "setboard":
                fen = command[9:]
                # To-do - test for legal position, and if illegal position, respond with tellusererror command
                try:
                    b.load_from_fen(fen)
                except:
                    print("Invalid FEN: {}".format(command[9:]))
            elif command == "undo":
                # take back a half move
                b.unapply_move()
            elif command == "remove":
                # take back a full move
                b.unapply_move()
                b.unapply_move()
            elif command[0:2] == "sd":
                try:
                    search_depth = int(command[3:])
                except:
                    print ("Invalid search depth: {}".format(command[3:]))
            elif command[0:2] == "st":
                search_time = int(command[3:]) * 1000  # milliseconds
            elif command == "draw":
                # for now, only draw we accept is due to 50-move rule
                # FIDE rule 9.3 - at move 50 without pawn move or capture, either side can claim a draw on their move.
                # Draw is automatic at move 75.  Move 50 = half-move 100.
                if True or b.halfmove_clock >= 100:
                    if XBOARD:
                        printcommand("offer draw")
                    else:
                        print("Draw claimed under 50-move rule.")
                    done_with_current_game = True
                else:
                    # for xboard do nothing, just don't accept it
                    if not XBOARD:
                        print("Draw invalid - halfmove clock only at: ", b.halfmove_clock)
            elif command == "history":
                print(b.print_move_history())
            elif command[0:6] == "result" or command[0:6] == "resign":
                # game is over, believe due to resignation
                done_with_current_game = True
            elif command == "post":
                POST = True
            elif command == "nopost":
                POST = False
            elif command == "fen":
                # this is command for terminal, not xboard
                print(b.convert_to_fen())
            elif command == "help":
                # this is a command for terminal, not xboard
                print_supported_commands()
            elif command == "print":
                # this is a command for terminal, not xboard
                print(b.pretty_print(True))
            elif command == "printpos":
                # this is a command for terminal, not xboard
                for piece in [WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK]:
                    tmpstr = piece_to_string_dict[piece] + ": "
                    if b.piece_count_locations[piece][0] == 0:
                        tmpstr += "[None]"
                    else:
                        for i in range(1, b.piece_count_location[piece][0] + 1):
                            loc = b.piece_count_locations[piece][i]
                            tmpstr += arraypos_to_algebraic(loc) + " "
                    print(tmpstr)
            elif command in ["random", "?", "hint", "hard", "easy", "computer"]:
                # treat these as no-ops
                pass
            elif command[0:4] == "name" or command[0:6] == "rating" or command[0:5] == "level"\
                    or command[0:8] == "accepted" or command[0:8] == "rejected":
                # no-op
                pass
            else:
                # we assume it is a move
                human_move = NULL_MOVE
                try:
                    human_move = return_validated_move(b, command)
                except:
                    printcommand("Error (unknown command): " + command)
                if human_move == NULL_MOVE:
                    printcommand("Illegal move: " + command)
                else:
                    # only add the FEN after the move so we don't slow the compute loop down by computing FEN's
                    # that we don't need.  We use the FEN only for draw-by-repetition testing outside the move loop.
                    # Inside computer computing moves, we use the hash for speed, giving up some accuracy.
                    b.apply_move(human_move)
                    b.required_post_move_updates()
                    if len(best_known_line) > 0:
                        if human_move != best_known_line[0]:
                            best_known_line = []
                        else:
                            best_known_line = best_known_line[1:]

cdef list global_movecount = []

cdef calc_moves(ChessBoard board, int depth, bint is_debug=False):

    global global_movecount, TRANSPOSITION_TABLE

    if depth == 0:
        return

    cached_ml = TRANSPOSITION_TABLE.probe(board)
    if cached_ml is None:
        ml = ChessMoveListGenerator(board)
        ml.generate_move_list()
        TRANSPOSITION_TABLE.insert(board, (ml.move_list,))

        local_move_list = ml.move_list
    else:
        local_move_list = cached_ml[0]

    global_movecount[depth-1] += len(local_move_list)


    if is_debug:

        for move in local_move_list:
            try:
                before_fen = board.convert_to_fen()
                board.apply_move(move)
                middle_fen = board.convert_to_fen()
            except:
                print("could not apply move %s to board %s" % (pretty_print_move(move), before_fen))
                print("history:")
                for x in board.move_history:
                    print(pretty_print_move(x[0]))
                raise

            try:
                calc_moves(board, depth-1)
            except:
                print("previous stack: depth %d applied %s to %s" % (depth, pretty_print_move(move), before_fen))
                raise

            try:
                board.unapply_move()
                after_fen = board.convert_to_fen()
            except:
                print("could not unapply move %s to board %s" % (pretty_print_move(move), after_fen))
                raise

            if before_fen != after_fen:
                print("%d : %s : %s : resulted in %s : then rolled back to : %s" % (depth, before_fen, pretty_print_move(move), middle_fen, after_fen))
                raise AssertionError("Halt")

    else:
        for move in local_move_list:
            board.apply_move(move)
            calc_moves(board, depth-1)
            board.unapply_move()


cdef perft_test(start_fen, validation_list, flush_cache_between_runs=True, is_debug=False):

    """
    :param start_fen: the FEN that you want to test
    :param validation_list: the expected answers at each depth.
    :param flush_cache_between_runs: True if you are doing performance testing, so board positions aren't held
            between executions of this command.  False if you just want to test for correctness.
    :param is_debug: In debug mode, we do an explicit test of the board position before apply and after
            unapply, enabling this to be used to test that routine as well.  It does slow things down a bit.
    :return: The results are printed to screen.
    """


    global global_movecount, TRANSPOSITION_TABLE
    if flush_cache_between_runs:
        TRANSPOSITION_TABLE = ChessPositionCache()
    global_movecount = []
    depth = len(validation_list)
    for i in range(depth):
        global_movecount.append(0)
    board = ChessBoard()
    board.load_from_fen(start_fen)

    start_time = datetime.now()
    try:
        calc_moves(board, depth, is_debug=is_debug)
    except:
        print("Failed to complete.  Elapsed time: %s" % (datetime.now() - start_time))
        raise

    end_time = datetime.now()
    print("Completed: %s -- Elapsed time: %s" % (start_fen, end_time-start_time))

    # Note my "depth" variable has depth 0 = the leaves, which is inverted of what people expect.
    # Reversing here for display purposes.
    validation_list.reverse()
    for i in range(depth-1, -1, -1):
        if global_movecount[i] == validation_list[i]:
            print("Ply %d: %d positions" % (depth-i, global_movecount[i]))
        else:
            print("Ply %d *ERROR*: Expected %d positions, got %d." % (depth-i, validation_list[i], global_movecount[i]))

def perft_series():
    # Copied from https://chessprogramming.wikispaces.com/Perft+Results
    perft_test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", [20, 400, 8902, 197281, 4865609])
    perft_test("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", [48, 2039, 97862, 4085603])
    perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", [14, 191, 2812, 43238, 674624, 11030083])
    perft_test("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", [6, 264, 9467, 422333, 15833292])
    perft_test("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", [6, 264, 9467, 422333, 15833292])
    perft_test("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ", [44, 1486, 62379, 2103487, 89941194])
    # perft_test("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ", [44, 1486, 62379, 2103487])
    perft_test("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", [46, 2079, 89890, 3894594])

    # Copied from https://github.com/thomasahle/sunfish/blob/master/tests/queen.fen
    perft_test("r1b2rk1/2p2ppp/p7/1p6/3P3q/1BP3bP/PP3QP1/RNB1R1K1 w - - 1 0", [40,1334,50182,1807137])


def start():

    # perft_series()
    play_game()

if __name__ == "__main__":
    play_game()
