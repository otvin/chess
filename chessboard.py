import random
import array
from copy import deepcopy
import movetable
from operator import xor
import colorama

# CONSTANTS for pieces.  7th bit is color
PAWN = 1
KNIGHT = 2
BISHOP = 4
ROOK = 8
QUEEN = 16
KING = 32
BLACK = 64
WHITE = 0

WP, BP = PAWN, BLACK | PAWN
WN, BN = KNIGHT, BLACK | KNIGHT
WB, BB = BISHOP, BLACK | BISHOP
WR, BR = ROOK, BLACK | ROOK
WQ, BQ = QUEEN, BLACK | QUEEN
WK, BK = KING, BLACK | KING
EMPTY = 0
OFF_BOARD = 128

# CONSTANTS for the bit field for attributes of the board.
W_CASTLE_QUEEN = 1
W_CASTLE_KING = 2
B_CASTLE_QUEEN = 4
B_CASTLE_KING = 8
W_TO_MOVE = 16
BOARD_IN_CHECK = 32

# constants that describe chess moves:
from chessmove_list import START, END, PIECE_MOVING, PIECE_CAPTURED, CAPTURE_DIFFERENTIAL, PROMOTED_TO, MOVE_FLAGS, \
                            MOVE_CASTLE, MOVE_EN_PASSANT, MOVE_CHECK, MOVE_DOUBLE_PAWN
from chessmove_list import pretty_print_move



def get_random_board_mask():
    retlist = []
    for i in range(120):
        # this could be 64, but I don't want to have to map a 120-square representation to a 64-square representation
        # when computing a hash
        retlist.append(random.getrandbits(64))
    return retlist

class ChessPositionCache:

    def __init__(self, cachesize=1048799):   # 1,299,827 is prime as is 251,611

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
        self.cache = [None] * cachesize
        self.inserts = 0
        self.probe_hits = 0


    def compute_hash(self, board):
        hash = 0
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

        return hash

    def debug_analyze_hash_differences(self, hash, bch):
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


    def insert(self, board, stuff):
        # We use "replace always."

        hash = board.cached_hash
        hash_modded = hash % self.cachesize
        self.cache[hash_modded] = (hash, stuff)
        self.inserts += 1


    def probe(self, board):
        # Returns the "stuff" that was cached if board exactly matches what is in the cache.
        # After testing 2 caches, one based on "depth" and "age" and the other "always replace," found that
        # We spent much more time in maintaining the two caches than we saved by having the additional information.
        # This is consistent with other tools with caches of the size we have.  The 2-cache strategy did better
        # when dealing with lots more collisions and smaller caches.

        hash = board.cached_hash
        hash_modded = hash % self.cachesize
        c = self.cache[hash_modded]

        if c is None:
            return None
        elif hash == c[0]:
            self.probe_hits += 1
            return c[1]
        else:
            return None


TRANSPOSITION_TABLE = ChessPositionCache()

# Helper functions

def algebraic_to_arraypos(algebraicpos):
    """
    :param algebraicpos: from "a1".."h8"
    :return: an integer from 0..119 = space in the array corresponding to the square represented by algebraicpos
    """

    assert(len(algebraicpos) == 2)
    assert(algebraicpos[1].isnumeric())

    file = algebraicpos[0].lower()
    rank = int(algebraicpos[1])

    assert('a' <= file <= 'h')
    assert(1 <= rank <= 8)

    retval = (10 * (rank+1)) + 1
    retval += (ord(file)-97)  # 97 is the ascii for "a"
    return retval


def arraypos_to_algebraic(arraypos):
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


def arraypos_is_on_board(arraypos):

    # layout of the board - count this way from 0..119

    # 110 111 112 113 114 115 116 117 118 119
    # 100 101 102 103 104 105 106 107 108 109
    # ...
    # 10 11 12 13 14 15 16 17 18 19
    # 00 01 02 03 04 05 06 07 08 09

    # all squares with 0 or 9 in the 1's space are off the board
    # all squares with tens digit 0 or 1 (which includes 100 and 110) are off the board

    retval = True

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

ob = -32767  # short for "off board"

white_pawn_pst = [
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

white_knight_pst = [
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

white_bishop_pst = [
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

white_rook_pst = [
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

white_queen_pst = [
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
white_king_pst = [
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

white_king_endgame_pst = [
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

# will initialize these programmatically later
black_pawn_pst = list(120 * " ")
black_knight_pst = list(120 * " ")
black_bishop_pst = list(120 * " ")
black_rook_pst = list(120 * " ")
black_queen_pst = list(120 * " ")
black_king_pst = list(120 * " ")
black_king_endgame_pst = list(120 * " ")


def mirror_square(square):
    # input - a position on the board
    # output - the equivalent position for the other player.
    # e.g. a1 mirrors a8.  f4 mirrors f5.
    # file stays the same, rank becomes 9-rank.  Ranks run from 20-90 not 0-70, so math here is 11 - rank

    return (10 * (11 - (square // 10))) + (square % 10)



# for KP vs K endgame, for each pawn position, identify the key squares, where if the King occupies them, the
# pawn will promote, regardless who moves next, unless enemy king is adjacent to pawn and it is enemy king's move
# See: https://en.wikipedi.org/wiki/King_and_pawn_versus_king_endgame
white_kpk_key_squares = {
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

black_kpk_key_squares = {
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


piece_value_dict = {WP: 100, BP: 100, WN: 320, BN: 320, WB: 330, BB: 330, WR: 500, BR: 500,
                    WQ: 900, BQ: 900, WK: 20000, BK: 20000}

white_piece_list = [WP, WN, WB, WR, WQ, WK]
black_piece_list = [BP, BN, BB, BR, BQ, BK]

piece_to_string_dict = {WP: "P", WN: "N", WB: "B", WR: "R", WQ: "Q", WK: "K", BP: "p", BN: "n", BB: "b", BR: "r",
                        BQ: "q", BK: "k", EMPTY: " ", OFF_BOARD: "x"}
string_to_piece_dict = {"P": WP, "N": WN, "B": WB, "R": WR, "Q": WQ, "K": WK, "p": BP, "n": BN, "b": BB, "r": BR,
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


# moving the hash positions from a dict/list combination to an array.  This will be modestly better performing
# in python but much better in Cython since it will use an actual C array.
HISTORICAL_HASH_ARRAY = [0] * 150
HISTORICAL_HASH_POSITION = -1  # we increment before we add to the array.
HISTORICAL_HASH_MIN_POSITION = 0  # the position where the halfmove clock was last set to 0.

class ChessBoard:

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
        self.piece_count = {BP: 0, WP: 0, BN: 0, WN: 0, BB: 0, WB: 0, BR: 0, WR: 0, BQ: 0, WQ: 0}
        self.initialize_piece_locations()
        self.cached_fen = self.convert_to_fen()
        self.cached_hash_dict = {}
        self.cached_hash_dict_position = 0
        self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)
        HISTORICAL_HASH_ARRAY = [0] * 150
        HISTORICAL_HASH_POSITION = -1  # we increment before we add to the array.
        HISTORICAL_HASH_MIN_POSITION = 0  # the position where the halfmove clock was last set to 0.

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
        self.piece_count = {BP: 8, WP: 8, BN: 2, WN: 2, BB: 2, WB: 2, BR: 2, WR: 2, BQ: 1, WQ: 1}
        self.initialize_piece_locations()
        self.cached_fen = self.convert_to_fen()
        self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)

    def initialize_piece_locations(self):
        self.piece_locations = {BP: [], WP: [], BN: [], WN: [], BB: [], WB: [], BR: [], WR: [],
                                BQ: [], WQ: [], BK: [], WK: []}
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank+file]
                if piece:
                    self.piece_locations[piece].append(rank+file)

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

        TRANSPOSITION_TABLE = ChessPositionCache()

        cached_array = deepcopy(self.board_array)
        cached_attributes = self.board_attributes
        cached_epts = self.en_passant_target_square
        cached_halfmove_clock = self.halfmove_clock
        cached_fmn = self.fullmove_number
        cached_move_hist = deepcopy(self.move_history)
        cached_historical_array = HISTORICAL_HASH_ARRAY
        cached_historical_arraypos = HISTORICAL_HASH_POSITION
        cached_historical_minarraypos = HISTORICAL_HASH_MIN_POSITION

        self.erase_board()

        try:
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
            self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)
            HISTORICAL_HASH_ARRAY[0] = [self.cached_hash]
            HISTORICAL_HASH_POSITION = 0
        except:
            # restore sanity
            self.board_array = deepcopy(cached_array)
            self.board_attributes = cached_attributes
            self.en_passant_target_square = cached_epts
            self.halfmove_clock = cached_halfmove_clock
            self.fullmove_number = cached_fmn
            self.initialize_piece_locations()
            self.move_history = deepcopy(cached_move_hist)
            self.cached_hash = TRANSPOSITION_TABLE.compute_hash(self)
            HISTORICAL_HASH_ARRAY = cached_historical_array
            HISTORICAL_HASH_POSITION = cached_historical_arraypos
            HISTORICAL_HASH_MIN_POSITION = cached_historical_minarraypos
            raise




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
            movestr = pretty_print_move(move[0], is_san = True)
            if halfmove % 2 == 0:
                outstr += movestr + "\n"
            else:
                numspaces = 10 - len(movestr)
                outstr += movestr + (numspaces * " ")
        return outstr

    def threefold_repetition(self):
        global HISTORICAL_HASH_POSITION, HISTORICAL_HASH_ARRAY, HISTORICAL_HASH_MIN_POSITION

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


    def evaluate_board(self):
        """

        :return: white score minus black score if white to move, otherwise the other way around.
        """
        white_majors = self.piece_count[WQ] + self.piece_count[WR]
        white_minors = self.piece_count[WB] + self.piece_count[WN]
        black_majors = self.piece_count[BQ] + self.piece_count[BR]
        black_minors = self.piece_count[BB] + self.piece_count[BN]


        if self.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            return 0  # Draw
        elif white_majors + white_minors + black_majors + black_minors + self.piece_count[WP] + self.piece_count[BP] == 0:
            return 0  # king vs. king = draw
        elif self.threefold_repetition():
            return 0 # Draw
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

                if black_majors + black_minors + white_majors + white_minors == 0:
                    # KP vs. KP optimization - white first
                    # This is an oversimplification for multiple reasons:
                    # 1) It doesn't penalize doubled-up pawns
                    # 2) It doesn't look to ensure that the pawn has no opposing pawn in the way (e.g. it doesn't
                    #    ensure that it's a passed pawn.
                    # In the first case, it's hard for a king to be in the key square of 2 pawns, and in the second
                    #   if we get the king to the key square, then the king will be able to capture any pawn that is
                    #   in the way.
                    for white_pawn in self.piece_locations[WP]:
                        for key_square in white_kpk_key_squares[white_pawn]:
                            if self.board_array[key_square] == WK:
                                late_game_white_king += 75
                                break
                    for black_pawn in self.piece_locations[BP]:
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

    def unapply_move(self):

        global HISTORICAL_HASH_POSITION, HISTORICAL_HASH_ARRAY
        move, attrs, ep_target, halfmove_clock, fullmove_number, cached_fen, cached_hash = self.move_history.pop()
        start, end, piece_moved, piece_captured, capture_diff, promoted_to, move_flags = move


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

        HISTORICAL_HASH_ARRAY[HISTORICAL_HASH_POSITION] = 0
        HISTORICAL_HASH_POSITION -= 1

    def apply_move(self, move):
        global TRANSPOSITION_TABLE, HISTORICAL_HASH_ARRAY, HISTORICAL_HASH_POSITION

        # this function doesn't validate that the move is legal, just applies the move

        start, end, piece_moving, piece_captured, capture_diff, promoted_to, move_flags = move

        self.move_history.append((move, self.board_attributes, self.en_passant_target_square,
                                  self.halfmove_clock, self.fullmove_number, self.cached_fen, self.cached_hash))


        self.piece_locations[piece_moving].remove(start)
        self.piece_locations[piece_moving].append(end)

        self.board_array[end] = piece_moving
        self.board_array[start] = EMPTY
        self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[piece_moving][start]
        self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[piece_moving][end]

        # Remove captured pawn if en passant capture
        if piece_captured:
            if move_flags & MOVE_EN_PASSANT:
                if piece_moving & BLACK:
                    ppos = end+10
                    # black is moving, blank out the space 10 more than destination space
                    self.piece_locations[WP].remove(ppos)
                    self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[WP][ppos]
                    self.board_array[ppos] = EMPTY
                    self.piece_count[WP] -= 1
                else:
                    ppos = end-10
                    self.piece_locations[BP].remove(ppos)
                    self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[BP][ppos]
                    self.board_array[ppos] = EMPTY
                    self.piece_count[BP] -= 1
            else:
                try:
                    self.piece_locations[piece_captured].remove(end)
                    self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[piece_captured][end]
                    self.piece_count[piece_captured] -= 1
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
                self.piece_locations[WR].remove(28)
                self.board_array[28] = EMPTY
                self.piece_locations[WR].append(26)
                self.board_array[26] = WR
                self.cached_hash ^= TRANSPOSITION_TABLE.whitecastleking
                if self.board_attributes & W_CASTLE_QUEEN:
                    self.cached_hash ^= TRANSPOSITION_TABLE.whitecastlequeen
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[WR][28]
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[WR][26]
            elif end == 23:  # white, queen side
                # assert self.white_can_castle_queen_side
                self.piece_locations[WR].remove(21)
                self.board_array[21] = EMPTY
                self.piece_locations[WR].append(24)
                self.board_array[24] = WR
                self.cached_hash ^= TRANSPOSITION_TABLE.whitecastlequeen
                if self.board_attributes & W_CASTLE_KING:
                    self.cached_hash ^= TRANSPOSITION_TABLE.whitecastleking
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[WR][21]
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[WR][24]
            elif end == 97:  # black, king side
                # assert self.black_can_castle_king_side
                self.piece_locations[BR].remove(98)
                self.board_array[98] = EMPTY
                self.piece_locations[BR].append(96)
                self.board_array[96] = BR
                self.cached_hash ^= TRANSPOSITION_TABLE.blackcastleking
                if self.board_attributes & B_CASTLE_QUEEN:
                    self.cached_hash ^= TRANSPOSITION_TABLE.blackcastlequeen
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[BR][98]
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[BR][96]
            elif end == 93:  # black, queen side
                # assert self.black_can_castle_queen_side
                self.piece_locations[BR].remove(91)
                self.board_array[91] = EMPTY
                self.piece_locations[BR].append(94)
                self.board_array[94] = BR
                self.cached_hash ^= TRANSPOSITION_TABLE.blackcastlequeen
                if self.board_attributes & B_CASTLE_KING:
                    self.cached_hash ^= TRANSPOSITION_TABLE.blackcastleking
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[BR][91]
                self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[BR][94]
            else:
                raise ValueError("Invalid Castle Move ", start, end)
        elif promoted_to:
            self.piece_locations[piece_moving].remove(end)
            self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[piece_moving][end]
            self.piece_count[piece_moving] -= 1
            self.board_array[end] = promoted_to
            self.cached_hash ^= TRANSPOSITION_TABLE.board_mask_dict[promoted_to][end]
            self.piece_locations[promoted_to].append(end)
            self.piece_count[promoted_to] += 1

        elif move_flags & MOVE_DOUBLE_PAWN:
            if piece_moving == WP:
                self.en_passant_target_square = end - 10
            else:
                self.en_passant_target_square = end + 10
            self.cached_hash ^= TRANSPOSITION_TABLE.enpassanttarget[self.en_passant_target_square]

        # other conditions to end castling - could make this more efficient
        oldattrs = self.board_attributes
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


    def find_piece(self, piece):
        return self.piece_locations[piece]

    def old_side_to_move_is_in_check(self):

        if self.board_attributes & W_TO_MOVE:
            king_position = self.piece_locations[WK][0]
            enemy_pawn, enemy_bishop, enemy_knight, enemy_queen, enemy_rook, enemy_king = BP, BB, BN, BQ, BR, BK
        else:
            king_position = self.piece_locations[BK][0]
            enemy_pawn, enemy_bishop, enemy_knight, enemy_queen, enemy_rook, enemy_king = WP, WB, WN, WQ, WR, WK

        for velocity in [-9, -11, 9, 11]:  # look for bishops and queens first
            cur_pos = king_position + velocity
            if self.board_array[cur_pos] == enemy_king:
                return True
            while self.board_array[cur_pos] == EMPTY:
                cur_pos += velocity
            if self.board_array[cur_pos] in [enemy_queen, enemy_bishop]:
                return True

        for velocity in [-10, -1, 1, 10]:  # look for rooks and queens next
            cur_pos = king_position + velocity
            if self.board_array[cur_pos] == enemy_king:
                return True
            while self.board_array[cur_pos] == EMPTY:
                cur_pos += velocity
            if self.board_array[cur_pos] in [enemy_queen, enemy_rook]:
                return True

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for cur_pos in [king_position + 8, king_position - 8, king_position + 12, king_position - 12,
                        king_position + 19, king_position - 19, king_position + 21, king_position - 21]:
            if self.board_array[cur_pos] == enemy_knight:
                return True

        # pawn checks
        if self.board_attributes & W_TO_MOVE:
            if self.board_array[king_position + 9] == BP or self.board_array[king_position + 11] == BP:
                return True
        else:
            if self.board_array[king_position - 9] == WP or self.board_array[king_position - 11] == WP:
                return True

        return False

    def side_to_move_is_in_check(self):

        if self.board_attributes & W_TO_MOVE:
            # The white and black loops are the same except for the test for is piece of the enemy color.
            # By separating this I can save one comparison per loop.  There may be a bitwise way to do this,
            # by xoring just the color bit with the color that is moving, but for now I will deal with the
            # extra long code.

            attack_list = movetable.WHITE_CHECK_TABLE[self.piece_locations[WK][0]]
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
            attack_list = movetable.BLACK_CHECK_TABLE[self.piece_locations[BK][0]]
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

    def generate_pinned_list(self, for_defense = True):
        # If we are looking at defense, we are generating the list of pieces that are pinned.
        # that is, a friendly piece blocking an enemy attacker from a friendly king.
        # If we are looking for attack, we are generating the list of pieces which, if moved,
        # would lead to discovered checks.  So this is a friendly piece, blocking a friendly attacker from the
        # enemy king.
        retlist = []

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

        king_position = self.piece_locations[defending_king][0]
        attacking_bishop = BISHOP | attacking_piece_color
        attacking_rook = ROOK | attacking_piece_color
        attacking_queen = QUEEN | attacking_piece_color


        test_sliders = False
        test_diagonals = False
        if len(self.piece_locations[attacking_queen]) > 0:
            test_sliders = True
            test_diagonals = True
        else:
            if len(self.piece_locations[attacking_bishop]) > 0:
                test_diagonals = True
            if len(self.piece_locations[attacking_rook]) > 0:
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

    def required_post_move_updates(self):
        # These are required updates after the real move is made.  These updates are not needed during move
        # evaluation, so are left out of apply/unapply move for performance reasons.
        global HISTORICAL_HASH_MIN_POSITION, HISTORICAL_HASH_POSITION

        self.cached_fen = self.convert_to_fen(True)
        if self.halfmove_clock == 0:
            self.cached_hash_dict = {}
            HISTORICAL_HASH_MIN_POSITION = HISTORICAL_HASH_POSITION
        else:
            if self.cached_hash in self.cached_hash_dict.keys():
                self.cached_hash_dict[self.cached_hash] += 1
            else:
                self.cached_hash_dict[self.cached_hash] = 1

        self.cached_hash_dict_position = len(self.move_history) - 1

