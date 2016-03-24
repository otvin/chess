import array
import movetable
from operator import xor
import colorama

# constants that describe chess moves:
from chessmove_list import START, END, PIECE_MOVING, PIECE_CAPTURED, CAPTURE_DIFFERENTIAL, PROMOTED_TO, MOVE_FLAGS, \
                            MOVE_CASTLE, MOVE_EN_PASSANT, MOVE_CHECK, MOVE_DOUBLE_PAWN


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

# will initialize these programmatically later
black_pawn_pst = list(120 * " ")
black_knight_pst = list(120 * " ")
black_bishop_pst = list(120 * " ")
black_rook_pst = list(120 * " ")
black_queen_pst = list(120 * " ")
black_king_pst = list(120 * " ")

# CONSTANTS for pieces.  7th bit is color
PAWN = 1
KNIGHT = 2
BISHOP = 4
ROOK = 8
QUEEN = 16
KING = 32
BLACK = 64

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


class ChessBoard:

    def __init__(self):
        self.board_array = list(120 * " ")

        # Originally, I had nice member variables for these.  However due to performance I'm trying to simplify
        # data storage.
        self.board_attributes = W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN | W_TO_MOVE

        self.en_passant_target_square = -1
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.move_history = []
        initialize_psts()
        self.pst_dict = {BP: black_pawn_pst, WP: white_pawn_pst, BB: black_bishop_pst, WB: white_bishop_pst,
                         BN: black_knight_pst, WN: white_knight_pst, BR: black_rook_pst, WR: white_rook_pst,
                         BQ: black_queen_pst, WQ: white_queen_pst, BK: black_king_pst, WK: white_king_pst}

        self.piece_count = {BP: 0, WP: 0, BN: 0, WN: 0, BB: 0, WB: 0, BR: 0, WR: 0, BQ: 0, WQ: 0}
        self.piece_locations = {BP: [], WP: [], BN: [], WN: [], BB: [], WB: [], BR: [], WR: [],
                                BQ: [], WQ: [], BK: [], WK: []}

        self.position_score = 0

        self.erase_board()

    def erase_board(self):
        for square in range(120):
            if arraypos_is_on_board(square):
                self.board_array[square] = EMPTY
            else:
                self.board_array[square] = OFF_BOARD

        self.board_attributes = W_CASTLE_KING | W_CASTLE_QUEEN | B_CASTLE_KING | B_CASTLE_QUEEN | W_TO_MOVE
        self.en_passant_target_square = -1
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.move_history = []
        self.piece_count = {BP: 0, WP: 0, BN: 0, WN: 0, BB: 0, WB: 0, BR: 0, WR: 0, BQ: 0, WQ: 0}
        self.position_score = 0
        self.initialize_piece_locations()

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
        self.position_score = 0
        self.initialize_piece_locations()

    def initialize_piece_locations(self):
        self.piece_locations = {BP: [], WP: [], BN: [], WN: [], BB: [], WB: [], BR: [], WR: [],
                                BQ: [], WQ: [], BK: [], WK: []}
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank+file]
                if piece:
                    self.piece_locations[piece].append(rank+file)

    def quickstring(self):
        # need a quick-to-generate unique string for a board to use to verify cache hits or misses
        ret = array.array('B', self.board_array)
        if self.en_passant_target_square == -1:
            ret.append(255)
        else:
            ret.append(self.en_passant_target_square)
        ret.append(self.board_attributes)
        return ret.tostring()

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
                self.position_score += self.pst_dict[string_to_piece_dict[cur_char]][cur_square]
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
            self.en_passant_target_square = -1
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
        self.debug_force_recalculation_of_position_score()
        self.initialize_piece_locations()

    def convert_to_fen(self):

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

        if self.en_passant_target_square == -1:
            retval += "-"
        else:
            retval += arraypos_to_algebraic(self.en_passant_target_square)

        retval += " "

        retval += str(self.halfmove_clock) + " " + str(self.fullmove_number)

        return retval

    def debug_force_recalculation_of_position_score(self):
        # only used in testing and loading from FEN
        self.position_score = 0
        for rank in range(90, 10, -10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank + file]
                if piece != EMPTY:
                    self.position_score += self.pst_dict[piece][rank+file]

    def evaluate_board(self):
        """

        :return: white score minus black score
        """

        if self.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            return 0  # Draw
        elif (self.piece_count[WP] + self.piece_count[WB] + self.piece_count[WN] + self.piece_count[WR] +
                    self.piece_count[WQ] == 0) and (self.piece_count[BP] + self.piece_count[BB] +
                    self.piece_count[BN] + self.piece_count[BR] + self.piece_count[BQ] == 0):
            return 0  # king vs. king = draw
        else:
            return self.position_score

    def unapply_move(self):

        move, attrs, ep_target, halfmove_clock, fullmove_number = self.move_history.pop()
        start, end, piece_moved, piece_captured, capture_diff, promoted_to, move_flags = move

        # move piece back
        if promoted_to:
            if promoted_to & BLACK:
                self.board_array[start] = BP
                self.position_score += self.pst_dict[BP][start]
                self.piece_locations[BP].append(start)
                self.piece_count[BP] += 1
            else:
                self.board_array[start] = WP
                self.position_score += self.pst_dict[WP][start]
                self.piece_locations[WP].append(start)
                self.piece_count[WP] += 1
            self.piece_count[promoted_to] -= 1
            self.position_score -= self.pst_dict[promoted_to][end]
            self.piece_locations[promoted_to].remove(end)

        else:
            self.board_array[start] = piece_moved
            self.position_score -= self.pst_dict[piece_moved][end]
            self.piece_locations[piece_moved].remove(end)
            self.position_score += self.pst_dict[piece_moved][start]
            self.piece_locations[piece_moved].append(start)

        if piece_captured:
            # if it was a capture, replace captured piece
            if move_flags & MOVE_EN_PASSANT:
                self.en_passant_target_square = end
                self.board_array[end] = EMPTY
                if piece_captured == BP:
                    pdest = end-10
                    self.board_array[pdest] = BP
                    self.position_score += self.pst_dict[BP][pdest]
                    self.piece_locations[BP].append(pdest)
                else:
                    pdest = end+10
                    self.board_array[pdest] = WP
                    self.position_score += self.pst_dict[WP][pdest]
                    self.piece_locations[WP].append(pdest)
            else:
                self.board_array[end] = piece_captured
                self.position_score += self.pst_dict[piece_captured][end]
                self.piece_locations[piece_captured].append(end)
            self.piece_count[piece_captured] += 1
        else:
            self.board_array[end] = EMPTY

            if move_flags & MOVE_CASTLE:
                # need to move the rook back too
                if end == 27:  # white, king side
                    self.position_score += self.pst_dict[WR][28]
                    self.piece_locations[WR].append(28)
                    self.board_array[28] = WR
                    self.position_score -= self.pst_dict[WR][26]
                    self.piece_locations[WR].remove(26)
                    self.board_array[26] = EMPTY
                elif end == 23:  # white, queen side
                    self.position_score += self.pst_dict[WR][21]
                    self.piece_locations[WR].append(21)
                    self.board_array[21] = WR
                    self.position_score -= self.pst_dict[WR][24]
                    self.piece_locations[WR].remove(24)
                    self.board_array[24] = EMPTY
                elif end == 97:  # black, king side
                    self.position_score += self.pst_dict[BR][98]
                    self.piece_locations[BR].append(98)
                    self.board_array[98] = BR
                    self.position_score -= self.pst_dict[BR][96]
                    self.piece_locations[BR].remove(96)
                    self.board_array[96] = EMPTY
                elif end == 93:  # black, queen side
                    self.position_score += self.pst_dict[BR][91]
                    self.piece_locations[BR].append(91)
                    self.board_array[91] = BR
                    self.position_score -= self.pst_dict[BR][94]
                    self.piece_locations[BR].remove(94)
                    self.board_array[94] = EMPTY

        # reset settings
        self.board_attributes = attrs
        self.halfmove_clock = halfmove_clock
        self.fullmove_number = fullmove_number
        self.en_passant_target_square = ep_target

    def apply_move(self, move):
        # this function doesn't validate that the move is legal, just applies the move
        # the asserts are mostly for debugging, may want to remove for performance later.

        # move, settings, ep_target, halfmove_clock, fullmove_number = self.move_history.pop()
        self.move_history.append((move, self.board_attributes, self.en_passant_target_square,
                                  self.halfmove_clock, self.fullmove_number))

        start, end, piece_moving, piece_captured, capture_diff, promoted_to, move_flags = move

        self.position_score -= self.pst_dict[piece_moving][start]
        self.piece_locations[piece_moving].remove(start)
        self.position_score += self.pst_dict[piece_moving][end]
        self.piece_locations[piece_moving].append(end)

        self.board_array[end] = piece_moving
        self.board_array[start] = EMPTY

        # Remove captured pawn if en passant capture
        if piece_captured:
            if move_flags & MOVE_EN_PASSANT:
                if piece_moving & BLACK:
                    # black is moving, blank out the space 10 more than destination space
                    self.position_score -= self.pst_dict[WP][end+10]
                    self.piece_locations[WP].remove(end+10)
                    self.board_array[end+10] = EMPTY
                    self.piece_count[WP] -= 1
                else:
                    self.position_score -= self.pst_dict[BP][end-10]
                    self.piece_locations[BP].remove(end-10)
                    self.board_array[end-10] = EMPTY
                    self.piece_count[BP] -= 1
            else:
                self.position_score -= self.pst_dict[piece_captured][end]
                self.piece_locations[piece_captured].remove(end)
                self.piece_count[piece_captured] -= 1

        # Reset en_passant_target_square and set below if it needs to be
        self.en_passant_target_square = -1

        if move_flags & MOVE_CASTLE:
            # the move includes the king, need to move the rook
            if end == 27:  # white, king side
                # assert self.white_can_castle_king_side
                self.position_score -= self.pst_dict[WR][28]
                self.piece_locations[WR].remove(28)
                self.board_array[28] = EMPTY
                self.position_score += self.pst_dict[WR][26]
                self.piece_locations[WR].append(26)
                self.board_array[26] = WR
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
            elif end == 23:  # white, queen side
                # assert self.white_can_castle_queen_side
                self.position_score -= self.pst_dict[WR][21]
                self.piece_locations[WR].remove(21)
                self.board_array[21] = EMPTY
                self.position_score += self.pst_dict[WR][24]
                self.piece_locations[WR].append(24)
                self.board_array[24] = WR
                self.board_attributes &= ~(W_CASTLE_QUEEN | W_CASTLE_KING)
            elif end == 97:  # black, king side
                # assert self.black_can_castle_king_side
                self.position_score -= self.pst_dict[BR][98]
                self.piece_locations[BR].remove(98)
                self.board_array[98] = EMPTY
                self.position_score += self.pst_dict[BR][96]
                self.piece_locations[BR].append(96)
                self.board_array[96] = BR
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
            elif end == 93:  # black, queen side
                # assert self.black_can_castle_queen_side
                self.position_score -= self.pst_dict[BR][91]
                self.piece_locations[BR].remove(91)
                self.board_array[91] = EMPTY
                self.position_score += self.pst_dict[BR][94]
                self.piece_locations[BR].append(94)
                self.board_array[94] = BR
                self.board_attributes &= ~(B_CASTLE_QUEEN | B_CASTLE_KING)
            else:
                raise ValueError("Invalid Castle Move ", start, end)
        elif promoted_to:
            self.position_score -= self.pst_dict[piece_moving][end]
            self.piece_locations[piece_moving].remove(end)
            self.piece_count[piece_moving] -= 1
            self.board_array[end] = promoted_to
            self.position_score += self.pst_dict[promoted_to][end]
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

        if (piece_moving & PAWN) or piece_captured:
            self.halfmove_clock = 0
        else:
            self.halfmove_clock += 1

        if not self.board_attributes & W_TO_MOVE:
            self.fullmove_number += 1

        self.board_attributes ^= W_TO_MOVE

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

    def generate_pinned_piece_list(self):
        # A piece is pinned if there is a piece that would put the current king in check if that piece were removed
        retlist = []

        if self.board_attributes & W_TO_MOVE:
            king_position = self.piece_locations[WK][0]
            enemy_bishop, enemy_rook, enemy_queen = BB, BR, BQ
            friendly_piece_list = WP, WN, WB, WR, WQ
        else:
            king_position = self.piece_locations[BK][0]
            enemy_bishop, enemy_rook, enemy_queen = WB, WR, WQ
            friendly_piece_list = BP, BN, BB, BR, BQ

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

    def generate_discovered_check_list(self):
        # A piece could lead to discovered check if it is the same color as the side moving, and
        # it moving out of the way allows another piece to put the opposite king in check.
        # logic looks like the pinned list so we may be able to combine later.

        retlist = []

        if self.board_attributes & W_TO_MOVE:
            enemy_king = BK
            friendly_bishop, friendly_rook, friendly_queen = WB, WR, WQ
            friendly_piece_list = WP, WN, WB, WR, WQ
        else:
            enemy_king = WK
            friendly_bishop, friendly_rook, friendly_queen = BB, BR, BQ
            friendly_piece_list = BP, BN, BB, BR, BQ

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
