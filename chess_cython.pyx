# cython: language_level=3

import argparse

import signal
import sys
from datetime import datetime
from cpython cimport datetime

from copy import deepcopy
import random
import array
from cpython cimport array

from operator import xor
import colorama

# global variables

cdef:
    datetime.datetime START_TIME = datetime.now()
    bint DEBUG = False
    bint XBOARD = False
    bint POST = False
    long long NODES = 0
    long long HIT_LOW = 0
    long long HIT_HIGH = 0
    long long HIT_EXACT = 0
    long long HIT_LOWQ = 0
    long long HIT_HIGHQ = 0
    long long HIT_EXACTQ = 0



# constants for transposition table access
cdef:
    int CACHE_SCORE_EXACT = 0
    int CACHE_SCORE_HIGH = 1
    int CACHE_SCORE_LOW = 2

# for debugging cache usage
cdef:
    long CACHE_HI = 0
    long CACHE_LOW = 0
    long CACHE_EXACT = 0


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
    int W_CASTLE_QUEEN = 1
    int W_CASTLE_KING = 2
    int B_CASTLE_QUEEN = 4
    int B_CASTLE_KING = 8
    int W_TO_MOVE = 16
    int BOARD_IN_CHECK = 32

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

cdef list WHITE_CHECK_TABLE = [list() for l in range(120)]
cdef list BLACK_CHECK_TABLE = [list() for l in range(120)]

cdef list EMPTY_BOARD = list(
                'xxxxxxxxxx'
                'xxxxxxxxxx'
                'x        x'
                'x        x'
                'x        x'
                'x        x'
                'x        x'
                'x        x'
                'x        x'
                'x        x'
                'xxxxxxxxxx'
                'xxxxxxxxxx')



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

cdef list white_pawn_pst = [
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

cdef list white_knight_pst = [
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

cdef list white_bishop_pst = [
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

cdef list white_rook_pst = [
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

cdef list white_queen_pst = [
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
cdef list white_king_pst = [
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

cdef list white_king_endgame_pst = [
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
    list black_pawn_pst = list(120 * " ")
    list black_knight_pst = list(120 * " ")
    list black_bishop_pst = list(120 * " ")
    list black_rook_pst = list(120 * " ")
    list black_queen_pst = list(120 * " ")
    list black_king_pst = list(120 * " ")
    list black_king_endgame_pst = list(120 * " ")

    dict piece_value_dict = {WP: 100, BP: 100, WN: 320, BN: 320, WB: 330, BB: 330, WR: 500, BR: 500,
                       WQ: 900, BQ: 900, WK: 20000, BK: 20000}

    list white_piece_list = [WP, WN, WB, WR, WQ, WK]
    list black_piece_list = [BP, BN, BB, BR, BQ, BK]

    dict piece_to_string_dict = {WP: "P", WN: "N", WB: "B", WR: "R", WQ: "Q", WK: "K", BP: "p", BN: "n", BB: "b", BR: "r",
                            BQ: "q", BK: "k", EMPTY: " ", OFF_BOARD: "x"}
    dict string_to_piece_dict = {"P": WP, "N": WN, "B": WB, "R": WR, "Q": WQ, "K": WK, "p": BP, "n": BN, "b": BB, "r": BR,
                           "q": BQ, "k": BK, " ": EMPTY, "x": OFF_BOARD}


def debug_print_pst(pst, name):
    outstr = name + "\n"
    for rank in range(90, 10, -10):
        for file in range(1, 9, 1):
            outstr += str(pst[rank + file]) + ", "
        outstr += "\n"
    print(outstr)


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

    if is_debug:
        debug_print_pst(white_pawn_pst, "White Pawn")
        debug_print_pst(black_pawn_pst, "Black Pawn")
        debug_print_pst(white_knight_pst, "White Knight")
        debug_print_pst(black_knight_pst, "Black Knight")
        debug_print_pst(white_bishop_pst, "White Bishop")
        debug_print_pst(black_bishop_pst, "Black Bishop")
        debug_print_pst(white_rook_pst, "White Rook")
        debug_print_pst(black_rook_pst, "Black Rook")
        debug_print_pst(white_queen_pst, "White Queen")
        debug_print_pst(black_queen_pst, "Black Queen")
        debug_print_pst(white_king_pst, "White King")
        debug_print_pst(black_king_pst, "Black King")
        debug_print_pst(white_king_endgame_pst, "White King Endgame")
        debug_print_pst(black_king_endgame_pst, "Black King Endgame")

cdef list get_random_board_mask():
    cdef list retlist = []
    for i in range(120):
        # this could be 64, but I don't want to have to map a 120-square representation to a 64-square representation
        # when computing a hash
        retlist.append(random.getrandbits(64))
    return retlist

cdef class ChessPositionCache:

    cdef:
        public unsigned long whitetomove
        public unsigned long blacktomove
        public unsigned long whitecastleking
        public unsigned long whitecastlequeen
        public unsigned long blackcastleking
        public unsigned long blackcastlequeen

        public list enpassanttarget, whitep, blackp, whiten, blackn
        public list whiteb, blackb, whiter, blackr, whiteq, blackq, whitek, blackk
        public dict board_mask_dict
        public list deep_cache, new_cache

        public long long deep_inserts, deep_probe_hits, new_inserts, new_probe_hits

        public unsigned long cachesize
        public int age


    def __init__(self, cachesize=1048799):   # 1,299,827 is prime as is 251,611. 1,048,799 is smallest prime > 2^20

        self.whitetomove = random.getrandbits(64)
        self.blacktomove = random.getrandbits(64)
        self.whitecastleking = random.getrandbits(64)
        self.whitecastlequeen = random.getrandbits(64)
        self.blackcastleking = random.getrandbits(64)
        self.blackcastlequeen = random.getrandbits(64)
        self.enpassanttarget = get_random_board_mask()  # could limit this to 16 squares

        whitep = get_random_board_mask()
        blackp = get_random_board_mask()
        whiten = get_random_board_mask()
        blackn = get_random_board_mask()
        whiteb = get_random_board_mask()
        blackb = get_random_board_mask()
        whiter = get_random_board_mask()
        blackr = get_random_board_mask()
        whiteq = get_random_board_mask()
        blackq = get_random_board_mask()
        whitek = get_random_board_mask()
        blackk = get_random_board_mask()

        self.board_mask_dict = {WP: whitep, BP: blackp, WN: whiten, BN: blackn, WB: whiteb, BB: blackb,
                                WR: whiter, BR: blackr, WQ: whiteq, BQ: blackq, WK: whitek, BK: blackk}

        self.cachesize = cachesize
        self.deep_cache = [None] * cachesize
        self.new_cache = [None] * cachesize
        self.deep_inserts = 0
        self.new_inserts = 0
        self.deep_probe_hits = 0
        self.new_probe_hits = 0

        # When we make a move, in theory everything in the cache is valid, because it is just searching given
        # positions at a given depth.  However, if the original PV isn't taken, then the "depth" cache gets full
        # of stale entries.  Balance this by keeping the old information for searching purposes, but on insert
        # if the entry is older (lower age) than what is being inserted, the newer record takes precedence.
        self.age = 0


    cdef unsigned long compute_hash(self, board):
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
            for i in board.piece_locations[piece]:
                hash ^= self.board_mask_dict[piece][i]

        return hash % self.cachesize


    cdef insert(self, ChessBoard board, int depth, tuple stuff):
        # In the depth cache, we store the new record only if the depth is greater than what is there.
        # If it is not greater, then we store it in the new cache, which is in essence the "replace always."

        cdef:
            unsigned long hash
            array.array board_string, cache_board_string
            int cache_depth, cache_age
            tuple cache_stuff


        hash = self.compute_hash(board)
        board.cached_hash = hash

        if self.deep_cache[hash] is None:
            self.deep_cache[hash] = (board.quickstring(), depth, self.age, stuff)
            self.deep_inserts += 1
        else:
            board_string = board.quickstring()
            cache_board_string, cache_depth, cache_age, cache_stuff = self.deep_cache[hash]

            if depth >= cache_depth or self.age > cache_age:
                self.deep_cache[hash] = (board_string, depth, self.age, stuff)
                self.deep_inserts += 1
            elif board_string != cache_board_string:  # don't put a lesser version of the deep hash board in the new hash board
                self.new_cache[hash] = (board_string, stuff)  # don't waste the bits storing depth or iteration here.
                self.new_inserts += 1


    cdef tuple probe(self, ChessBoard board):
        # Returns the "stuff" that was cached if board exactly matches what is in the cache.
        # Deep cache is checked before new cache, as deep cache should have >= depth than new cache so is
        # more valuable.
        # If nothing is in the cache or it is a different board, returns None.

        cdef:
            unsigned long hash
            tuple c

        hash = self.compute_hash(board)
        board.cached_hash = hash
        if self.deep_cache[hash] is None:
            # deep_cache gets inserted first, so if no deep, then there is no new.
            return None
        else:
            c = self.deep_cache[hash]
            if board.quickstring() == c[0]:
                self.deep_probe_hits += 1
                return c[3]
            else:
                if self.new_cache[hash] is None:
                    return None
                else:
                    c = self.new_cache[hash]
                    if board.quickstring() == c[0]:
                        self.new_probe_hits += 1
                        return c[1]
        return None

    cdef void age_cache(self):
        self.age += 1

cdef ChessPositionCache global_chess_position_move_cache = ChessPositionCache()


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

"""

cdef list generate_attack_list(int start, int velocity, int pieces, bint include_pawn):
    cdef list retlist = []
    cdef int current = start + velocity
    if EMPTY_BOARD[current] != "x":
        # first square is unique because it includes the king, and possibly the pawn
        if include_pawn:
            retlist.append((current, pieces | KING | PAWN))
        else:
            retlist.append((current, pieces | KING))
        current += velocity
        while EMPTY_BOARD[current] != "x":
            retlist.append((current, pieces))
            current += velocity
    return retlist

cdef list generate_knight_list(int start):
    cdef list retlist = []
    cdef int delta
    for delta in [-21, -19, -12, -8, 8, 12, 19, 21]:
        if EMPTY_BOARD[start + delta] != "x":
            retlist.append((start + delta, KNIGHT))
    return retlist


def init_check_tables():

    for rank in range (20, 100, 10):
        for file in range (1, 9, 1):
            # Black and White use same slide moves and knight moves
            start = rank + file
            N = generate_attack_list(start, 10, ROOK | QUEEN, False)
            E = generate_attack_list(start, 1, ROOK | QUEEN, False)
            S = generate_attack_list(start, -10, ROOK | QUEEN, False)
            W = generate_attack_list(start, -1, ROOK | QUEEN, False)
            KNIGHTATTACK = generate_knight_list(start)

            # White king is attacked by pawns from the NE, NW; Black King from SE, SW
            WHITE_NE = generate_attack_list(start, 11, BISHOP | QUEEN, True)
            BLACK_NE = generate_attack_list(start, 11, BISHOP | QUEEN, False)
            WHITE_NW = generate_attack_list(start, 9, BISHOP | QUEEN, True)
            BLACK_NW = generate_attack_list(start, 9, BISHOP | QUEEN, False)

            WHITE_SE = generate_attack_list(start, -9, BISHOP | QUEEN, False)
            BLACK_SE = generate_attack_list(start, -9, BISHOP | QUEEN, True)
            WHITE_SW = generate_attack_list(start, -11, BISHOP | QUEEN, False)
            BLACK_SW = generate_attack_list(start, -11, BISHOP | QUEEN, True)

            cur_spot = 0
            white_main_list = []
            black_main_list = []
            for attack_list in [N, E, S, W]:
                # these are same for both colors
                next_spot = cur_spot + len(attack_list)
                for attack in attack_list:
                    white_main_list.append((attack[0], attack[1], next_spot))
                    black_main_list.append((attack[0], attack[1], next_spot))
                    cur_spot += 1

            white_cur_spot = cur_spot
            black_cur_spot = cur_spot

            for attack_list in [WHITE_NE, WHITE_NW, WHITE_SE, WHITE_SW]:
                next_spot = white_cur_spot + len(attack_list)
                for attack in attack_list:
                    white_main_list.append((attack[0], attack[1], next_spot))
                    white_cur_spot += 1

            for attack_list in [BLACK_NE, BLACK_NW, BLACK_SE, BLACK_SW]:
                next_spot = black_cur_spot + len(attack_list)
                for attack in attack_list:
                    black_main_list.append((attack[0], attack[1], next_spot))
                    black_cur_spot += 1

            assert (white_cur_spot == black_cur_spot)

            cur_spot = white_cur_spot
            for attack in KNIGHTATTACK:
                next_spot = cur_spot + 1
                white_main_list.append((attack[0], attack[1], next_spot))
                black_main_list.append((attack[0], attack[1], next_spot))
                cur_spot = cur_spot + 1

            white_main_list.append((None, None, None))
            black_main_list.append((None, None, None))

            WHITE_CHECK_TABLE[start] = white_main_list
            BLACK_CHECK_TABLE[start] = black_main_list

init_check_tables()


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
            int direction, i, listlen
            int testpos, blocker

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
            # Need to look every direction other than back towards where we came from to see if there is a check
            dirlist = [1, -1, 10, -10, 11, -11, 9, -9]
            dirlist.remove(-1 * velocity)
        else:
            # Two ways for the move to be a check.  First, we take the piece that was blocking us from check,
            # so look straight ahead.  Then, look perpendicular.  Cannot put the king into check behind us, else
            # king would have already been in check.
            dirlist = [velocity, perpendicular_velocity, -1 * perpendicular_velocity]

        listlen = len(ret_list)
        for i in range(listlen):
            move = ret_list[i]
            for direction in dirlist:
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
            int piece, destpos, delta, pos, testpos, targetsquare, capture_diff, listlen, i
            Move move

        if knight & BLACK:  # quicker than referencing the white_to_move in the board object
            enemy_list = white_piece_list
        else:
            enemy_list = black_piece_list

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for dest_pos in (start_pos-21, start_pos-19, start_pos-12, start_pos-8,
                         start_pos+21, start_pos+19, start_pos+12, start_pos+8):
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
            for delta in [-21, -19, -12, -8, 21, 19, 12, 8]:

                targetsquare = self.board.board_array[pos+delta]

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

        if king & BLACK:  # quicker than referencing the white_to_move in the board object
            enemy_list = white_piece_list
        else:
            enemy_list = black_piece_list

        for dest_pos in (start_pos-1, start_pos+9, start_pos+10, start_pos+11,
                         start_pos+1, start_pos-9, start_pos-10, start_pos-11):
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

    cdef generate_move_list(self, Move last_best_move=NULL_MOVE):
        """

        :param last_best_move: optional - if provided, was the last known best move for this position, and will end up
                first in the return list.
        :return: Updates the move_list member to the moves in order they should be searched.  Current heuristic is
                1) last_best_move goes first if present
                2) Captures go next, in order of MVV-LVA - most valuable victim minus least valuable aggressor
                3) Any moves that put the opponent in check
                4) Any other moves
        """
        cdef:
            list potential_list, capture_list, noncapture_list, check_list, priority_list
            list pinned_piece_list, discovered_check_list
            Move move, m
            int pos, en_passant_target_square
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

        pinned_piece_list = self.board.generate_pinned_piece_list()
        discovered_check_list = self.board.generate_discovered_check_list()

        currently_in_check = self.board.board_attributes & BOARD_IN_CHECK
        en_passant_target_square = self.board.en_passant_target_square

        if self.board.board_attributes & W_TO_MOVE:
            pawn, knight, bishop, rook, queen, king = WP, WN, WB, WR, WQ, WK
        else:
            pawn, knight, bishop, rook, queen, king = BP, BN, BB, BR, BQ, BK

        for piece in self.board.piece_locations[pawn]:
            potential_list += self.generate_pawn_moves(piece)
        for piece in self.board.piece_locations[knight]:
            # pinned knights can't move
            if piece not in pinned_piece_list:
                potential_list += self.generate_knight_moves(piece)
        for piece in self.board.piece_locations[bishop]:
            potential_list += self.generate_diagonal_moves(piece, bishop)
        for piece in self.board.piece_locations[rook]:
            potential_list += self.generate_slide_moves(piece, rook)
        for piece in self.board.piece_locations[queen]:
            potential_list += self.generate_diagonal_moves(piece, queen)
            potential_list += self.generate_slide_moves(piece, queen)

        potential_list += self.generate_king_moves(self.board.piece_locations[king][0], currently_in_check)

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
                    self.board.piece_locations[which_king_moving][0] = middle_square

                    self.board.board_array[end] = EMPTY
                    if self.board.side_to_move_is_in_check():
                        move_valid = False

                    # put the king and rook back where they would belong so that unapply move works properly
                    self.board.board_array[middle_square] = which_rook_moving
                    self.board.board_array[end] = which_king_moving
                    self.board.piece_locations[which_king_moving][0] = end

                self.board.board_attributes ^= W_TO_MOVE  # flip it to the side whose turn it really is

            if piece_captured & KING:
                for x in self.board.move_history:
                    print(pretty_print_move(x[0]))
                raise ValueError("CAPTURED KING - preceding move not detected as check or something")

            if move_valid:
                if (start in discovered_check_list) or promoted_to:
                    # we tested for all other checks when we generated the move
                    if self.board.side_to_move_is_in_check():
                        move |= (<long long>MOVE_CHECK << MOVE_FLAGS_SHIFT)
                        flags |= MOVE_CHECK
                if (last_best_move == move):
                    priority_list = [move]
                elif piece_captured:
                    capture_list += [move]
                elif flags & MOVE_CHECK:
                    check_list += [move]
                else:
                    noncapture_list += [move]

            self.board.unapply_move()

        capture_list.sort(key=lambda mymove: -1 * (mymove & CAPTURE_DIFFERENTIAL))
        self.move_list = priority_list + capture_list + check_list + noncapture_list





cdef class ChessBoard:

    cdef:
        public list board_array
        public unsigned int board_attributes
        public int en_passant_target_square
        public int halfmove_clock
        public int fullmove_number
        public str cached_fen
        public dict cached_hash_dict
        public int cached_hash_dict_position
        public unsigned long cached_hash
        public list move_history
        public dict pst_dict
        public dict piece_count
        public dict piece_locations


    def __init__(self):
        self.board_array = list(120 * " ")

        # Originally, I had nice member variables for these.  However due to performance I'm trying to simplify
        # data storage.
        self.board_attributes = W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN | W_TO_MOVE

        self.en_passant_target_square = 0
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.cached_fen = ""
        self.cached_hash_dict = {}  # for threefold repetition test performance
        self.cached_hash_dict_position = 0  # the spot in the move list where previous hashes are all in the dict.
        self.cached_hash = 0
        self.move_history = []
        initialize_psts()
        self.pst_dict = {BP: black_pawn_pst, WP: white_pawn_pst, BB: black_bishop_pst, WB: white_bishop_pst,
                         BN: black_knight_pst, WN: white_knight_pst, BR: black_rook_pst, WR: white_rook_pst,
                         BQ: black_queen_pst, WQ: white_queen_pst, BK: (black_king_pst, black_king_endgame_pst),
                         WK: (white_king_pst, white_king_endgame_pst)}

        self.piece_count = {BP: 0, WP: 0, BN: 0, WN: 0, BB: 0, WB: 0, BR: 0, WR: 0, BQ: 0, WQ: 0}
        self.piece_locations = {BP: [], WP: [], BN: [], WN: [], BB: [], WB: [], BR: [], WR: [],
                                BQ: [], WQ: [], BK: [], WK: []}
        self.erase_board()

    def erase_board(self):
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
        self.piece_count = {BP: 0, WP: 0, BN: 0, WN: 0, BB: 0, WB: 0, BR: 0, WR: 0, BQ: 0, WQ: 0}
        self.initialize_piece_locations()
        self.cached_fen = self.convert_to_fen()
        self.cached_hash_dict = {}
        self.cached_hash_dict_position = 0

    def initialize_start_position(self):
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
        self.piece_count = {BP: 8, WP: 8, BN: 2, WN: 2, BB: 2, WB: 2, BR: 2, WR: 2, BQ: 1, WQ: 1}
        self.initialize_piece_locations()
        self.cached_fen = self.convert_to_fen()

    def initialize_piece_locations(self):
        self.piece_locations = {BP: [], WP: [], BN: [], WN: [], BB: [], WB: [], BR: [], WR: [],
                                BQ: [], WQ: [], BK: [], WK: []}
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank+file]
                if piece:
                    self.piece_locations[piece].append(rank+file)

    cdef array.array quickstring(self):
        cdef array.array ret
        # need a quick-to-generate unique string for a board to use to verify cache hits or misses
        ret = array.array('B', self.board_array)
        ret.append(self.en_passant_target_square)
        ret.append(self.board_attributes)

        return ret

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
                if cur_char not in ["k", "K"]:
                    self.piece_count[string_to_piece_dict[cur_char]] += 1
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

    cpdef bint threefold_repetition(self):
        # similar to the logic we use in test_for_end() to enforce the threefold repetition rule, this
        # version uses cached hash values as the comparison should be faster than the fen

        cdef:
            dict hash_count_dict = {}
            tuple move_history_record
            int halfmove_clock
            unsigned long hashcache

        hash_count_dict = {}
        for move_history_record in reversed(self.move_history[self.cached_hash_dict_position:]):
            halfmove_clock, hashcache = move_history_record[3], move_history_record[6]
            if hashcache in hash_count_dict.keys():
                hash_count_dict[hashcache] += 1
                if hash_count_dict[hashcache] >= 3:
                    return True
            elif hashcache in self.cached_hash_dict.keys():
                hash_count_dict[hashcache] = self.cached_hash_dict[hashcache] + 1
                if hash_count_dict[hashcache] >= 3:
                    return True
            else:
                hash_count_dict[hashcache] = 1
            if halfmove_clock == 0:
                return False  # no draw by repetition
        return False


    cdef int evaluate_board(self):
        """

        :return: white score minus black score if white to move, otherwise the other way around.
        """
        cdef:
            int position_score, piece, square, phase, total_phase, wk_location, bk_location
            int early_game_white_king, late_game_white_king, early_game_black_king, late_game_black_king
            list locations
            double phase_pct, inv_phase_pct


        if self.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            return 0  # Draw
        elif (self.piece_count[WP] + self.piece_count[WB] + self.piece_count[WN] + self.piece_count[WR] +
                    self.piece_count[WQ] == 0) and (self.piece_count[BP] + self.piece_count[BB] +
                    self.piece_count[BN] + self.piece_count[BR] + self.piece_count[BQ] == 0):
            return 0  # king vs. king = draw
        elif self.threefold_repetition():
            return 0
        else:

            # tapered evaluation taken from https://chessprogramming.wikispaces.com/Tapered+Eval and modified

            position_score = 0
            for piece in [WP, WN, WB, WR, WQ, BP, BN, BB, BR, BQ]:
                locations = self.piece_locations[piece]
                for square in locations:
                    position_score += self.pst_dict[piece][square]


            # Kings have a separate score based on time of game

            total_phase = 24
            phase = total_phase
            phase -= (self.piece_count[BN] + self.piece_count[WN])
            phase -= (self.piece_count[BB] + self.piece_count[WB])
            phase -= (2 * (self.piece_count[BR] + self.piece_count[WR]))
            phase -= (4 * (self.piece_count[BQ] + self.piece_count[WQ]))

            # phase 0 = beginning
            # phase of total_phase = end
            # otherwise it is in the middle

            wk_location = self.piece_locations[WK][0]
            bk_location = self.piece_locations[BK][0]

            early_game_white_king = self.pst_dict[WK][0][wk_location]
            early_game_black_king = self.pst_dict[BK][0][bk_location]

            if phase == 0:
                position_score += early_game_white_king + early_game_black_king
            else:
                late_game_white_king = self.pst_dict[WK][1][wk_location]
                late_game_black_king = self.pst_dict[BK][1][bk_location]

                if phase >= 18:
                    # in late late game, reward kings for getting in good position vs. their pawns
                    if (self.board_array[wk_location-9] == WP or self.board_array[wk_location-10] == WP
                            or self.board_array[wk_location-11] == WP):
                        late_game_white_king += 30
                    elif self.board_attributes & W_TO_MOVE and (
                        self.board_array[wk_location-19] == WP or self.board_array[wk_location-20] == WP or
                        self.board_array[wk_location-21] == WP):
                        late_game_white_king += 20

                    if (self.board_array[bk_location-9] == BP or self.board_array[bk_location-10] == BP
                            or self.board_array[bk_location-11] == BP):
                        late_game_white_king += 30
                    elif (not self.board_attributes & W_TO_MOVE) and (
                        self.board_array[bk_location-19] == BP or self.board_array[bk_location-20] == BP or
                        self.board_array[bk_location-21] == BP):
                        late_game_white_king += 20

                # add phase/total * late + (1- (phase/total)) * early

                phase_pct = phase/total_phase
                inv_phase_pct = 1-phase_pct

                position_score += int(inv_phase_pct * (early_game_white_king + early_game_black_king))
                position_score += int(phase_pct * (late_game_white_king + late_game_black_king))

            if not (self.board_attributes & W_TO_MOVE):
                position_score = position_score * -1
            return position_score


    cdef unapply_move(self):

        cdef:
            Move move
            int attrs, ep_target, halfmove_clock, fullmove_number
            str cached_fen
            unsigned long cached_hash
            int start, end, piece_moved, piece_captured, capture_diff, promoted_to, move_flags

        move, attrs, ep_target, halfmove_clock, fullmove_number, cached_fen, cached_hash = self.move_history.pop()

        start = move & START
        end = ((move & END) >> END_SHIFT)
        piece_moved = ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT)
        piece_captured = ((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT)
        # capture_diff = ((move & CAPTURE_DIFFERENTIAL) >> CAPTURE_DIFFERENTIAL_SHIFT) - CAPTURE_DIFFERENTIAL_OFFSET
        promoted_to = ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT)
        move_flags = ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT)

        # move piece back
        if promoted_to:
            if promoted_to & BLACK:
                self.board_array[start] = BP
                self.piece_locations[BP].append(start)
                self.piece_count[BP] += 1
            else:
                self.board_array[start] = WP
                self.piece_locations[WP].append(start)
                self.piece_count[WP] += 1
            self.piece_count[promoted_to] -= 1
            self.piece_locations[promoted_to].remove(end)

        else:
            self.board_array[start] = piece_moved
            self.piece_locations[piece_moved].remove(end)
            self.piece_locations[piece_moved].append(start)

        if piece_captured:
            # if it was a capture, replace captured piece
            if move_flags & MOVE_EN_PASSANT:
                self.en_passant_target_square = end
                self.board_array[end] = EMPTY
                if piece_captured == BP:
                    pdest = end-10
                    self.board_array[pdest] = BP
                    self.piece_locations[BP].append(pdest)
                else:
                    pdest = end+10
                    self.board_array[pdest] = WP
                    self.piece_locations[WP].append(pdest)
            else:
                self.board_array[end] = piece_captured
                self.piece_locations[piece_captured].append(end)
            self.piece_count[piece_captured] += 1
        else:
            self.board_array[end] = EMPTY

            if move_flags & MOVE_CASTLE:
                # need to move the rook back too
                if end == 27:  # white, king side
                    self.piece_locations[WR].append(28)
                    self.board_array[28] = WR
                    self.piece_locations[WR].remove(26)
                    self.board_array[26] = EMPTY
                elif end == 23:  # white, queen side
                    self.piece_locations[WR].append(21)
                    self.board_array[21] = WR
                    self.piece_locations[WR].remove(24)
                    self.board_array[24] = EMPTY
                elif end == 97:  # black, king side
                    self.piece_locations[BR].append(98)
                    self.board_array[98] = BR
                    self.piece_locations[BR].remove(96)
                    self.board_array[96] = EMPTY
                elif end == 93:  # black, queen side
                    self.piece_locations[BR].append(91)
                    self.board_array[91] = BR
                    self.piece_locations[BR].remove(94)
                    self.board_array[94] = EMPTY

        # reset settings
        self.board_attributes = attrs
        self.halfmove_clock = halfmove_clock
        self.fullmove_number = fullmove_number
        self.en_passant_target_square = ep_target
        self.cached_fen = cached_fen
        self.cached_hash = cached_hash

    cdef apply_move(self, Move move):
        # this function doesn't validate that the move is legal, just applies the move
        # the asserts are mostly for debugging, may want to remove for performance later.

        cdef:
            int start, end, piece_moving, piece_captured, capture_diff, promoted_to, move_flags




        self.move_history.append((move, self.board_attributes, self.en_passant_target_square,
                                  self.halfmove_clock, self.fullmove_number, self.cached_fen, self.cached_hash))


        start = move & START
        end = ((move & END) >> END_SHIFT)
        piece_moving = ((move & PIECE_MOVING) >> PIECE_MOVING_SHIFT)
        piece_captured = ((move & PIECE_CAPTURED) >> PIECE_CAPTURED_SHIFT)
        # capture_diff = ((move & CAPTURE_DIFFERENTIAL) >> CAPTURE_DIFFERENTIAL_SHIFT) - CAPTURE_DIFFERENTIAL_OFFSET
        promoted_to = ((move & PROMOTED_TO) >> PROMOTED_TO_SHIFT)
        move_flags = ((move & MOVE_FLAGS) >> MOVE_FLAGS_SHIFT)

        self.piece_locations[piece_moving].remove(start)
        self.piece_locations[piece_moving].append(end)

        self.board_array[end] = piece_moving
        self.board_array[start] = EMPTY

        # Remove captured pawn if en passant capture
        if piece_captured:
            if move_flags & MOVE_EN_PASSANT:
                if piece_moving & BLACK:
                    # black is moving, blank out the space 10 more than destination space
                    self.piece_locations[WP].remove(end+10)
                    self.board_array[end+10] = EMPTY
                    self.piece_count[WP] -= 1
                else:
                    self.piece_locations[BP].remove(end-10)
                    self.board_array[end-10] = EMPTY
                    self.piece_count[BP] -= 1
            else:
                self.piece_locations[piece_captured].remove(end)
                try:
                    self.piece_count[piece_captured] -= 1
                except:
                    print(self.print_move_history())
                    print(self.pretty_print(True))
                    raise

        # Reset en_passant_target_square and set below if it needs to be
        self.en_passant_target_square = 0

        if move_flags & MOVE_CASTLE:
            # the move includes the king, need to move the rook
            if end == 27:  # white, king side
                # assert self.white_can_castle_king_side
                self.piece_locations[WR].remove(28)
                self.board_array[28] = EMPTY
                self.piece_locations[WR].append(26)
                self.board_array[26] = WR
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
            elif end == 23:  # white, queen side
                # assert self.white_can_castle_queen_side
                self.piece_locations[WR].remove(21)
                self.board_array[21] = EMPTY
                self.piece_locations[WR].append(24)
                self.board_array[24] = WR
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
            elif end == 97:  # black, king side
                # assert self.black_can_castle_king_side
                self.piece_locations[BR].remove(98)
                self.board_array[98] = EMPTY
                self.piece_locations[BR].append(96)
                self.board_array[96] = BR
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
            elif end == 93:  # black, queen side
                # assert self.black_can_castle_queen_side
                self.piece_locations[BR].remove(91)
                self.board_array[91] = EMPTY
                self.piece_locations[BR].append(94)
                self.board_array[94] = BR
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
            else:
                raise ValueError("Invalid Castle Move ", start, end)
        elif promoted_to:
            self.piece_locations[piece_moving].remove(end)
            self.piece_count[piece_moving] -= 1
            self.board_array[end] = promoted_to
            self.piece_locations[promoted_to].append(end)
            self.piece_count[promoted_to] += 1

        elif move_flags & MOVE_DOUBLE_PAWN:
            if piece_moving == WP:
                self.en_passant_target_square = end - 10
            else:
                self.en_passant_target_square = end + 10

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

        if (piece_moving & PAWN) or piece_captured:
            self.halfmove_clock = 0
        else:
            self.halfmove_clock += 1

        if not self.board_attributes & W_TO_MOVE:
            self.fullmove_number += 1

        self.board_attributes ^= W_TO_MOVE

    cdef find_piece(self, int piece):
        return self.piece_locations[piece]

    cdef bint side_to_move_is_in_check(self):

        cdef:
            list attack_list = []
            int curpos, occupant
            tuple cur_attack

        if self.board_attributes & W_TO_MOVE:
            # The white and black loops are the same except for the test for is piece of the enemy color.
            # By separating this I can save one comparison per loop.  There may be a bitwise way to do this,
            # by xoring just the color bit with the color that is moving, but for now I will deal with the
            # extra long code.

            attack_list = WHITE_CHECK_TABLE[self.piece_locations[WK][0]]
            curpos = 0
            cur_attack = attack_list[0]
            while cur_attack[0] is not None:
                occupant = self.board_array[cur_attack[0]]
                if occupant & cur_attack[1]:
                    # is the piece of the enemy color?
                    if occupant & BLACK:
                        return True  # bust out of this loop
                    else:
                        curpos = cur_attack[2]  # this direction is blocked
                elif occupant:
                    curpos = cur_attack[2]
                else:
                    curpos += 1

                cur_attack = attack_list[curpos]
        else:
            attack_list = BLACK_CHECK_TABLE[self.piece_locations[BK][0]]
            curpos = 0
            cur_attack = attack_list[0]
            while cur_attack[0] is not None:
                occupant = self.board_array[cur_attack[0]]
                if occupant & cur_attack[1]:
                    # is the piece of the enemy color?
                    if not(occupant & BLACK):
                        return True  # bust out of this loop
                    else:
                        curpos = cur_attack[2]  # this direction is blocked
                elif occupant:
                    curpos = cur_attack[2]
                else:
                    curpos += 1

                cur_attack = attack_list[curpos]

        return False

    cdef list generate_pinned_piece_list(self):
        # A piece is pinned if there is a piece that would put the current king in check if that piece were removed

        cdef:
            list retlist = [], friendly_piece_list
            int king_position, enemy_bishop, enemy_rook, enemy_queen, velocity, cur_pos, pinning_pos

        if self.board_attributes & W_TO_MOVE:
            king_position = self.piece_locations[WK][0]
            enemy_bishop, enemy_rook, enemy_queen = BB, BR, BQ
            friendly_piece_list = [WP, WN, WB, WR, WQ]
        else:
            king_position = self.piece_locations[BK][0]
            enemy_bishop, enemy_rook, enemy_queen = WB, WR, WQ
            friendly_piece_list = [BP, BN, BB, BR, BQ]

        for velocity in [-9, -11, 9, 11]:
            cur_pos = king_position + velocity
            while self.board_array[cur_pos] == EMPTY:
                cur_pos += velocity
            if self.board_array[cur_pos] in friendly_piece_list:
                # now keep going to see if a bishop or queen of the opposite color is the next piece we see
                pinning_pos = cur_pos + velocity
                while self.board_array[pinning_pos] == EMPTY:
                    pinning_pos += velocity
                if self.board_array[pinning_pos] in [enemy_queen, enemy_bishop]:
                    retlist.append(cur_pos)

        for velocity in [-10, -1, 1, 10]:
            cur_pos = king_position + velocity
            while self.board_array[cur_pos] == EMPTY:
                cur_pos += velocity
            if self.board_array[cur_pos] in friendly_piece_list:
                # now keep going to see if a bishop or queen of the opposite color is the next piece we see
                pinning_pos = cur_pos + velocity
                while self.board_array[pinning_pos] == EMPTY:
                    pinning_pos += velocity
                if self.board_array[pinning_pos] in [enemy_queen, enemy_rook]:
                    retlist.append(cur_pos)

        return retlist

    cdef list generate_discovered_check_list(self):
        # A piece could lead to discovered check if it is the same color as the side moving, and
        # it moving out of the way allows another piece to put the opposite king in check.
        # logic looks like the pinned list so we may be able to combine later.

        cdef:
            list retlist = [], friendly_piece_list
            int enemy_king, friendly_bishop, friendly_rook, friendly_queen, enemy_king_position, cur_pos, velocity
            int pinning_pos

        if self.board_attributes & W_TO_MOVE:
            enemy_king = BK
            friendly_bishop, friendly_rook, friendly_queen = WB, WR, WQ
            friendly_piece_list = [WP, WN, WB, WR, WQ, WK]
        else:
            enemy_king = WK
            friendly_bishop, friendly_rook, friendly_queen = BB, BR, BQ
            friendly_piece_list = [BP, BN, BB, BR, BQ, BK]

        enemy_king_position = self.piece_locations[enemy_king][0]

        for velocity in [-9, -11, 9, 11]:
            cur_pos = enemy_king_position + velocity
            while not self.board_array[cur_pos]:
                cur_pos += velocity
            if self.board_array[cur_pos] in friendly_piece_list:
                pinning_pos = cur_pos + velocity
                while not self.board_array[pinning_pos]:
                    pinning_pos += velocity
                if self.board_array[pinning_pos] in [friendly_queen, friendly_bishop]:
                    retlist.append(cur_pos) # if this piece moves, you get a discovered check from the queen/bishop

        for velocity in [-10, -1, 1, 10]:
            cur_pos = enemy_king_position + velocity
            while not self.board_array[cur_pos]:
                cur_pos += velocity
            if self.board_array[cur_pos] in friendly_piece_list:
                pinning_pos = cur_pos + velocity
                while not self.board_array[pinning_pos]:
                    pinning_pos += velocity
                if self.board_array[pinning_pos] in [friendly_queen, friendly_rook]:
                    retlist.append(cur_pos)

        return retlist


    cdef required_post_move_updates(self):
        # These are required updates after the real move is made.  These updates are not needed during move
        # evaluation, so are left out of apply/unapply move for performance reasons.
        self.cached_fen = self.convert_to_fen(True)
        if self.halfmove_clock == 0:
            self.cached_hash_dict = {}
        else:
            if self.cached_hash in self.cached_hash_dict.keys():
                self.cached_hash_dict[self.cached_hash] += 1
            else:
                self.cached_hash_dict[self.cached_hash] = 1

        self.cached_hash_dict_position = len(self.move_history)


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

    global NODES, DEBUG, POST, global_chess_position_move_cache
    global CACHE_HI, CACHE_LOW, CACHE_EXACT

    cdef:
        long original_alpha, cache_score, best_score, score
        int cache_depth, cache_score_type
        tuple cached_position, tmptuple
        list cached_ml, cached_opponent_movelist, move_list, best_move_sequence, move_sequence
        ChessMoveListGenerator move_list_generator
        Move my_best_move, move, previous_best_move

    # Pseudocode can be found at: https://en.wikipedia.org/wiki/Negamax

    # TO-DO:  Insert "if we have exceeded our maximum time, return, setting some flag that we quit the search.
    # Even though we won't be able to finish the ply, we will have built out the transposition cache.

    NODES += 1
    original_alpha = alpha

    if board.threefold_repetition():
        return 0, [NULL_MOVE]  # Draw - stop searching this position.  Do not cache, as transposition may not always be draw

    cached_position = global_chess_position_move_cache.probe(board)
    if cached_position is not None:
        cached_ml, cache_depth, cache_score, cache_score_type, cached_opponent_movelist = cached_position

        if cache_depth >= depth:
            if cache_score_type == CACHE_SCORE_EXACT:
                CACHE_EXACT += 1
                return cache_score, cached_opponent_movelist
            elif cache_score_type == CACHE_SCORE_LOW:
                if cache_score > alpha:
                    CACHE_LOW += 1
                    alpha = cache_score
                    best_known_line = cached_opponent_movelist
            elif cache_score_type == CACHE_SCORE_HIGH:
                if cache_score < beta:
                    CACHE_HI += 1
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

    else:
        if len(best_known_line) > 0:
            previous_best_move = best_known_line[0]
        else:
            previous_best_move = NULL_MOVE

        move_list_generator = ChessMoveListGenerator(board)
        move_list_generator.generate_move_list(previous_best_move)
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
            break  # alpha beta cutoff

    if best_score <= original_alpha:
        cache_score_type = CACHE_SCORE_HIGH
    elif best_score >= beta:
        cache_score_type = CACHE_SCORE_LOW
    else:
        cache_score_type = CACHE_SCORE_EXACT

    global_chess_position_move_cache.insert(board, depth, (move_list, depth, best_score, cache_score_type,
                                                           [my_best_move] + best_move_sequence))

    return best_score, [my_best_move] + best_move_sequence

cdef list process_computer_move(ChessBoard board, list best_known_line, int search_depth=4, long search_time=10000):
    global START_TIME, XBOARD
    global CACHE_HI, CACHE_LOW, CACHE_EXACT, NODES
    global global_chess_position_move_cache

    cdef:
        long best_score
        datetime.timedelta delta
        datetime.datetime end_time
        Move computer_move
        str movestr, movetext


    START_TIME = datetime.now()
    if not XBOARD:
        if board.side_to_move_is_in_check():
            print("Check!")


    CACHE_HI, CACHE_LOW, CACHE_EXACT, NODES = 0, 0, 0, 0

    # age the deep cache
    global_chess_position_move_cache.age_cache()
    best_score, best_known_line = negamax_recurse(board, search_depth, -101000, 101000, search_depth, best_known_line)
    print ("NODES:%d CACHE HI:%d  CACHE_LOW:%d  CACHE EXACT:%d" % (NODES, CACHE_HI, CACHE_LOW, CACHE_EXACT))

    assert(len(best_known_line) > 0)

    end_time = datetime.now()
    computer_move = best_known_line[0]

    if not XBOARD:
        print("Elapsed time: " + str(end_time - START_TIME))
        print("Move made: %s : Score = %d" % (pretty_print_move(computer_move, True), best_score))
        movestr = ""
        print ("E%d: H%d: L%d: EQ:%d HQ:%d LQ:%d" % (HIT_EXACT, HIT_HIGH, HIT_LOW, HIT_EXACTQ, HIT_HIGHQ, HIT_LOWQ))
        for c in best_known_line:
            movestr += pretty_print_move(c) + " "
        print(movestr)
        if DEBUG:
            print("Board pieces:", board.piece_count)

    if XBOARD:
        movetext = arraypos_to_algebraic(computer_move & START)
        movetext += arraypos_to_algebraic((computer_move & END) >> END_SHIFT)
        if computer_move & PROMOTED_TO:
            movetext += piece_to_string_dict[(computer_move & PROMOTED_TO) >> PROMOTED_TO_SHIFT].lower()
        printcommand("move " + movetext)

    board.apply_move(computer_move)

    # We use the best_known_line to order moves in the next search if the opponent takes the next move in the line
    return best_known_line[1:]


# Required to handle these signals if you want to use xboard

def handle_sigint(signum, frame):
    global DEBUGFILE
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGINT at " + str(datetime.now()) + "\n")


def handle_sigterm(signum, frame):
    global DEBUGFILE
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGTERM at " + str(datetime.now()) + "\n")
    sys.exit()


cdef bint test_for_end(ChessBoard board):

    cdef:
        dict fen_count_list
        tuple history_record
        int halfmove_lock
        str fen
        ChessMoveListGenerator move_list

    # Test for draw by repetition:
    fen_count_list = {}
    for history_record in reversed(board.move_history):
        halfmove_clock, fen = history_record[3], history_record[5]
        if halfmove_clock == 0:
            break # no draw by repetition.
        if fen in fen_count_list.keys():
            fen_count_list[fen] += 1
            if fen_count_list[fen] >= 3:
                printcommand("1/2-1/2 {Stalemate - Repetition}")
                return True
        else:
            fen_count_list[fen] = 1

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
    elif (board.piece_count[WP] + board.piece_count[WB] + board.piece_count[WN] + board.piece_count[WR] +
                board.piece_count[WQ] == 0) and (board.piece_count[BP] + board.piece_count[BB] +
                board.piece_count[BN] + board.piece_count[BR] + board.piece_count[BQ] == 0):
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


cpdef play_game(str debugfen=""):

    cdef:
        ChessBoard b
        bint computer_is_black, computer_is_white, done_with_current_game
        int search_depth, piece, loc
        long search_time
        Move human_move
        str command, tmpstr
        list best_known_line = []



    global DEBUG, XBOARD, POST, NODES, DEBUGFILE, global_chess_position_move_cache
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
    search_time = 10000  # milliseconds

    expected_opponent_move = NULL_MOVE
    counter_to_expected_opp_move = NULL_MOVE

    done_with_current_game = False
    while True:
        print("Deep Cache inserts: %d  Deep Cache hits: %d" % (global_chess_position_move_cache.deep_inserts, global_chess_position_move_cache.deep_probe_hits))
        print("New Cache inserts: %d  New Cache hits: %d" % (global_chess_position_move_cache.new_inserts, global_chess_position_move_cache.new_probe_hits))

        # only use the expected opponent move / counter if computer vs. human.
        if (computer_is_black and computer_is_white) or (not computer_is_black and not computer_is_white):
            expected_opponent_move = NULL_MOVE
            counter_to_expected_opp_move = NULL_MOVE

        # Check for mate/stalemate
        if not done_with_current_game:
            done_with_current_game = test_for_end(b)
        if computer_is_black and computer_is_white:
            if done_with_current_game:
                computer_is_black = False
                computer_is_white = False
            else:
                best_known_line = process_computer_move(b, best_known_line, search_depth)
                b.required_post_move_updates()
                if not XBOARD:
                    print(b.pretty_print(True))
                best_known_line = process_computer_move(b, best_known_line, search_depth)
                b.required_post_move_updates()
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
                printcommand('feature myname="Bejola0.5"')
                printcommand("feature ping=1")
                printcommand("feature setboard=1")
                printcommand("feature san=0")
                printcommand("feature sigint=1")
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
                NODES = 0
                best_known_line = process_computer_move(b, best_known_line, search_depth)
                b.required_post_move_updates()
            elif command[0:8] == "setboard":
                fen = command[9:]
                # To-do - test for legal position, and if illegal position, respond with tellusererror command
                b.load_from_fen(fen)
            elif command == "undo":
                # take back a half move
                b.unapply_move()
            elif command == "remove":
                # take back a full move
                b.unapply_move()
                b.unapply_move()
            elif command[0:2] == "sd":
                search_depth = int(command[3:])
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
                for piece in b.piece_locations.keys():
                    tmpstr = piece_to_string_dict[piece] + ": "
                    if len(b.piece_locations[piece]) == 0:
                        tmpstr += "[None]"
                    else:
                        for loc in b.piece_locations[piece]:
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
                    if ((b.board_attributes & W_TO_MOVE) and computer_is_white) or \
                            ((not (b.board_attributes & W_TO_MOVE)) and computer_is_black):
                        done_with_current_game = test_for_end(b)
                        if not done_with_current_game:
                            NODES = 0
                            best_known_line = process_computer_move(b, best_known_line, search_depth)
                            b.required_post_move_updates()


cdef list global_movecount = []
cdef ChessPositionCache global_movecache = ChessPositionCache()

cdef calc_moves(ChessBoard board, int depth, bint is_debug=False):

    global global_movecount, global_movecache

    if depth == 0:
        return

    cached_ml = global_movecache.probe(board)
    if cached_ml is None:
        ml = ChessMoveListGenerator(board)
        ml.generate_move_list()
        global_movecache.insert(board, depth, ml.move_list)
        local_move_list = ml.move_list
    else:
        local_move_list = cached_ml

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


    global global_movecount, global_movecache
    if flush_cache_between_runs:
        global_movecache = ChessPositionCache()
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
    # print("Cache inserts: %d, Cache probe hits %d" % (global_movecache.inserts, global_movecache.probe_hits))

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
    perft_test("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", [46, 2079, 89890, 3894594])

    # Copied from https://github.com/thomasahle/sunfish/blob/master/tests/queen.fen
    perft_test("r1b2rk1/2p2ppp/p7/1p6/3P3q/1BP3bP/PP3QP1/RNB1R1K1 w - - 1 0", [40,1334,50182,1807137])






def start():

    # perft_series()
    play_game()



if __name__ == "__main__":
    play_game()
