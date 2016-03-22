import colorama
from operator import xor


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


class ChessBoardMemberCache:
    def __init__(self, board):
        self.white_can_castle_queen_side = board.white_can_castle_queen_side
        self.white_can_castle_king_side = board.white_can_castle_king_side
        self.black_can_castle_queen_side = board.black_can_castle_queen_side
        self.black_can_castle_king_side = board.black_can_castle_king_side
        self.en_passant_target_square = board.en_passant_target_square
        self.halfmove_clock = board.halfmove_clock
        self.fullmove_number = board.fullmove_number

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

piece_value_dict = {"P": 100, "p": 100, "N": 320, "n": 320, "B": 330, "b": 330, "R": 500, "r": 500,
                    "Q": 900, "q": 900, "K": 20000, "k": 20000}

white_piece_list = ["P", "N", "B", "R", "Q", "K"]
black_piece_list = ["p", "n", "b", "r", "q", "k"]


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
            white_pawn_pst[rank + file] += piece_value_dict["P"]
            white_knight_pst[rank + file] += piece_value_dict["N"]
            white_bishop_pst[rank + file] += piece_value_dict["B"]
            white_rook_pst[rank + file] += piece_value_dict["R"]
            white_queen_pst[rank + file] += piece_value_dict["Q"]
            white_king_pst[rank + file] += piece_value_dict["K"]

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
        self.white_can_castle_queen_side = True
        self.white_can_castle_king_side = True
        self.black_can_castle_queen_side = True
        self.black_can_castle_king_side = True
        self.white_to_move = True
        self.en_passant_target_square = -1
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.move_history = []
        initialize_psts()
        self.pst_dict = {"p": black_pawn_pst, "P": white_pawn_pst, "b": black_bishop_pst, "B": white_bishop_pst,
                         "n": black_knight_pst, "N": white_knight_pst, "r": black_rook_pst, "R": white_rook_pst,
                         "q": black_queen_pst, "Q": white_queen_pst, "k": black_king_pst, "K": white_king_pst}

        self.piece_count = {"p": 0, "P": 0, "n": 0, "N": 0, "b": 0, "B": 0, "r": 0, "R": 0, "q": 0, "Q": 0}
        self.piece_locations = {"p": [], "P": [], "n": [], "N": [], "b": [], "B": [], "r": [], "R": [],
                                "q": [], "Q": [], "k": [], "K": []}

        self.position_score = 0

        # For cleaner code, I would prefer to have init only call erase, but need to define all members in __init__
        self.erase_board()

    def erase_board(self):
        self.board_array = list(
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

        self.white_can_castle_queen_side = True
        self.white_can_castle_king_side = True
        self.black_can_castle_queen_side = True
        self.black_can_castle_king_side = True
        self.white_to_move = True
        self.en_passant_target_square = -1
        self.halfmove_clock = 0
        self.fullmove_number = 1
        self.move_history = []
        self.piece_count = {"p": 0, "P": 0, "n": 0, "N": 0, "b": 0, "B": 0, "r": 0, "R": 0, "q": 0, "Q": 0}
        self.position_score = 0
        self.initialize_piece_locations()

    def initialize_start_position(self):
        self.erase_board()
        # NOTE - you are looking at a mirror image up/down of the board below, the first line is the bottom of the board
        self.board_array = list(
            'xxxxxxxxxx'
            'xxxxxxxxxx'
            'xRNBQKBNRx'
            'xPPPPPPPPx'
            'x        x'
            'x        x'
            'x        x'
            'x        x'
            'xppppppppx'
            'xrnbqkbnrx'
            'xxxxxxxxxx'
            'xxxxxxxxxx')

        self.piece_count = {"p": 8, "P": 8, "n": 2, "N": 2, "b": 2, "B": 2, "r": 2, "R": 2, "q": 1, "Q": 1}
        self.position_score = 0
        self.initialize_piece_locations()

    def initialize_piece_locations(self):
        self.piece_locations = {"p": [], "P": [], "n": [], "N": [], "b": [], "B": [], "r": [], "R": [],
                                "q": [], "Q": [], "k": [], "K": []}
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank+file]
                if piece != " ":
                    self.piece_locations[piece].append(rank+file)

    def quickstring(self):
        # need a quick-to-generate unique string for a board
        return "".join(self.board_array) + str(self.en_passant_target_square) + str(self.black_can_castle_king_side) + \
            str(self.black_can_castle_queen_side) + str(self.white_can_castle_queen_side) + \
            str(self.black_can_castle_king_side)
        # to-do test with removing all the "x" characters, to see if that makes it faster or slower


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

                    if piece in ("p", "n", "b", "r", "q", "k"):
                        outstr += colorama.Fore.BLUE
                    else:
                        outstr += colorama.Fore.GREEN

                    outstr += " " + piece + " "
                else:
                    if piece == " ":
                        outstr += "."
                    else:
                        outstr += piece

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
                self.board_array[cur_square] = cur_char
                self.position_score += self.pst_dict[cur_char][cur_square]
                if cur_char.lower() != "k":
                    self.piece_count[cur_char] += 1
                cur_square += 1

        counter = fen.find(" ") + 1
        if fen[counter] == "w":
            self.white_to_move = True
        else:
            self.white_to_move = False

        counter += 2
        self.black_can_castle_king_side = False
        self.black_can_castle_queen_side = False
        self.white_can_castle_king_side = False
        self.white_can_castle_queen_side = False
        while fen[counter] != " ":
            if fen[counter] == "K":
                self.white_can_castle_king_side = True
            elif fen[counter] == "Q":
                self.white_can_castle_queen_side = True
            elif fen[counter] == "k":
                self.black_can_castle_king_side = True
            elif fen[counter] == "q":
                self.black_can_castle_queen_side = True
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
                if self.board_array[rank + file] == " ":
                    num_blanks += 1
                else:
                    if num_blanks > 0:
                        retval += str(num_blanks)
                        num_blanks = 0
                    retval += self.board_array[rank + file]

            if num_blanks > 0:
                retval += str(num_blanks)
            if rank > 20:
                retval += "/"

        retval += " "

        if self.white_to_move:
            retval += "w"
        else:
            retval += "b"

        retval += " "

        castle_string = ""
        if self.white_can_castle_king_side:
            castle_string += "K"
        if self.white_can_castle_queen_side:
            castle_string += "Q"
        if self.black_can_castle_king_side:
            castle_string += "k"
        if self.black_can_castle_queen_side:
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

    def pos_occupied_by_color_moving(self, pos):
        retval = False
        piece = self.board_array[pos]
        if piece != " " and piece != "x":
            if self.white_to_move:
                if piece.isupper():
                    retval = True
            else:
                if piece.islower():
                    retval = True
        return retval

    def pos_occupied_by_color_not_moving(self, pos):
        global black_piece_list
        global white_piece_list
        if self.white_to_move:
            return self.board_array[pos] in black_piece_list
        else:
            return self.board_array[pos] in white_piece_list

    def debug_force_recalculation_of_position_score(self):
        # only used in testing.
        self.position_score = 0
        for rank in range(90, 10, -10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank + file]
                if piece != " ":
                    self.position_score += self.pst_dict[piece][rank+file]

    def evaluate_board(self):
        """

        :return: white score minus black score
        """

        if self.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            return 0  # Draw
        elif (self.piece_count["P"] + self.piece_count["B"] + self.piece_count["N"] + self.piece_count["R"] +
                    self.piece_count["Q"] == 0) and (self.piece_count["p"] + self.piece_count["b"] +
                    self.piece_count["n"] + self.piece_count["r"] + self.piece_count["q"] == 0):
            return 0  # king vs. king = draw
        else:
            return self.position_score

    def unapply_move(self):

        move, cache = self.move_history.pop()

        # move piece back
        if move.is_promotion:
            if move.promoted_to.islower():
                self.board_array[move.start] = "p"
                self.position_score += self.pst_dict["p"][move.start]
                self.piece_locations["p"].append(move.start)
                self.piece_count["p"] += 1
            else:
                self.board_array[move.start] = "P"
                self.position_score += self.pst_dict["P"][move.start]
                self.piece_locations["P"].append(move.start)
                self.piece_count["P"] += 1
            self.piece_count[move.promoted_to] -= 1
            self.position_score -= self.pst_dict[move.promoted_to][move.end]
            self.piece_locations[move.promoted_to].remove(move.end)

        else:
            piece_moved = self.board_array[move.end]
            self.board_array[move.start] = piece_moved
            self.position_score -= self.pst_dict[piece_moved][move.end]
            self.piece_locations[piece_moved].remove(move.end)
            self.position_score += self.pst_dict[piece_moved][move.start]
            self.piece_locations[piece_moved].append(move.start)

        if move.is_capture:
            # if it was a capture, replace captured piece
            if move.is_en_passant_capture:
                self.en_passant_target_square = move.end
                self.board_array[move.end] = " "
                if move.piece_captured == "p":
                    pdest = move.end-10
                    self.board_array[pdest] = "p"
                    self.position_score += self.pst_dict["p"][pdest]
                    self.piece_locations["p"].append(pdest)
                else:
                    pdest = move.end+10
                    self.board_array[pdest] = "P"
                    self.position_score += self.pst_dict["P"][pdest]
                    self.piece_locations["P"].append(pdest)
            else:
                self.board_array[move.end] = move.piece_captured
                self.position_score += self.pst_dict[move.piece_captured][move.end]
                self.piece_locations[move.piece_captured].append(move.end)
            self.piece_count[move.piece_captured] += 1
        else:
            self.board_array[move.end] = " "

            if move.is_castle:
                # need to move the rook back too
                if move.end == 27:  # white, king side
                    self.position_score += self.pst_dict["R"][28]
                    self.piece_locations["R"].append(28)
                    self.board_array[28] = "R"
                    self.position_score -= self.pst_dict["R"][26]
                    self.piece_locations["R"].remove(26)
                    self.board_array[26] = " "
                elif move.end == 23:  # white, queen side
                    self.position_score += self.pst_dict["R"][21]
                    self.piece_locations["R"].append(21)
                    self.board_array[21] = "R"
                    self.position_score -= self.pst_dict["R"][24]
                    self.piece_locations["R"].remove(24)
                    self.board_array[24] = " "
                elif move.end == 97:  # black, king side
                    self.position_score += self.pst_dict["r"][98]
                    self.piece_locations["r"].append(98)
                    self.board_array[98] = "r"
                    self.position_score -= self.pst_dict["r"][96]
                    self.piece_locations["r"].remove(96)
                    self.board_array[96] = " "
                elif move.end == 93:  # black, queen side
                    self.position_score += self.pst_dict["r"][91]
                    self.piece_locations["r"].append(91)
                    self.board_array[91] = "r"
                    self.position_score -= self.pst_dict["r"][94]
                    self.piece_locations["r"].remove(94)
                    self.board_array[94] = " "

        # reset settings
        self.white_can_castle_queen_side = cache.white_can_castle_queen_side
        self.white_can_castle_king_side = cache.white_can_castle_king_side
        self.black_can_castle_queen_side = cache.black_can_castle_queen_side
        self.black_can_castle_king_side = cache.black_can_castle_king_side
        self.halfmove_clock = cache.halfmove_clock
        self.fullmove_number = cache.fullmove_number
        self.en_passant_target_square = cache.en_passant_target_square

        self.white_to_move = not self.white_to_move

    def apply_move(self, move):
        # assert(self.board_array[move.start] != " ")  # make sure start isn't empty
        # assert(self.board_array[move.start] != "x")  # make sure start isn't off board
        # assert(self.board_array[move.end] != "x")  # make sure end isn't off board

        # this function doesn't validate that the move is legal, just applies the move
        # the asserts are mostly for debugging, may want to remove for performance later.

        self.move_history.append((move, ChessBoardMemberCache(self)))

        piece_moving = self.board_array[move.start]

        self.position_score -= self.pst_dict[piece_moving][move.start]
        self.piece_locations[piece_moving].remove(move.start)
        self.position_score += self.pst_dict[piece_moving][move.end]
        self.piece_locations[piece_moving].append(move.end)

        self.board_array[move.end] = piece_moving
        self.board_array[move.start] = " "

        # Remove captured pawn if en passant capture
        if move.is_capture:
            if move.end == self.en_passant_target_square and piece_moving in ["p", "P"]:
                if piece_moving == "P":
                    # white is moving, blank out the space 10 less than destination space
                    self.position_score -= self.pst_dict["p"][self.en_passant_target_square-10]
                    self.piece_locations["p"].remove(self.en_passant_target_square-10)
                    self.board_array[self.en_passant_target_square-10] = " "
                    self.piece_count["p"] -= 1
                else:
                    self.position_score -= self.pst_dict["P"][self.en_passant_target_square+10]
                    self.piece_locations["P"].remove(self.en_passant_target_square+10)
                    self.board_array[self.en_passant_target_square+10] = " "
                    self.piece_count["P"] -= 1
            else:
                self.position_score -= self.pst_dict[move.piece_captured][move.end]
                self.piece_locations[move.piece_captured].remove(move.end)
                self.piece_count[move.piece_captured] -= 1

        # Reset en_passant_target_square and set below if it needs to be
        self.en_passant_target_square = -1

        if move.is_castle:
            # the move includes the king, need to move the rook
            if move.end == 27:  # white, king side
                # assert self.white_can_castle_king_side
                self.position_score -= self.pst_dict["R"][28]
                self.piece_locations["R"].remove(28)
                self.board_array[28] = " "
                self.position_score += self.pst_dict["R"][26]
                self.piece_locations["R"].append(26)
                self.board_array[26] = "R"
                self.white_can_castle_king_side = False
                self.white_can_castle_queen_side = False
            elif move.end == 23:  # white, queen side
                # assert self.white_can_castle_queen_side
                self.position_score -= self.pst_dict["R"][21]
                self.piece_locations["R"].remove(21)
                self.board_array[21] = " "
                self.position_score += self.pst_dict["R"][24]
                self.piece_locations["R"].append(24)
                self.board_array[24] = "R"
                self.white_can_castle_king_side = False
                self.white_can_castle_queen_side = False
            elif move.end == 97:  # black, king side
                # assert self.black_can_castle_king_side
                self.position_score -= self.pst_dict["r"][98]
                self.piece_locations["r"].remove(98)
                self.board_array[98] = " "
                self.position_score += self.pst_dict["r"][96]
                self.piece_locations["r"].append(96)
                self.board_array[96] = "r"
                self.black_can_castle_king_side = False
                self.black_can_castle_queen_side = False
            elif move.end == 93:  # black, queen side
                # assert self.black_can_castle_queen_side
                self.position_score -= self.pst_dict["r"][91]
                self.piece_locations["r"].remove(91)
                self.board_array[91] = " "
                self.position_score += self.pst_dict["r"][94]
                self.piece_locations["r"].append(94)
                self.board_array[94] = "r"
                self.black_can_castle_queen_side = False
                self.black_can_castle_king_side = False
            else:
                raise ValueError("Invalid Castle Move ", move.start, move.end)
        elif move.is_promotion:
            if self.white_to_move:
                self.position_score -= self.pst_dict["P"][move.end]
                self.piece_locations["P"].remove(move.end)
                self.piece_count["P"] -= 1
                move.promoted_to = move.promoted_to.upper()
            else:
                self.position_score -= self.pst_dict["p"][move.end]
                self.piece_locations["p"].remove(move.end)
                self.piece_count["p"] -= 1
                move.promoted_to = move.promoted_to.lower()
            self.board_array[move.end] = move.promoted_to
            self.position_score += self.pst_dict[move.promoted_to][move.end]
            self.piece_locations[move.promoted_to].append(move.end)
            self.piece_count[move.promoted_to] += 1

        elif move.is_two_square_pawn_move:
            if piece_moving == "P":
                self.en_passant_target_square = move.end - 10
            else:
                self.en_passant_target_square = move.end + 10

        # other conditions to end castling - could make this more efficient
        if self.white_can_castle_king_side or self.white_can_castle_king_side:
            if piece_moving == "K":
                self.white_can_castle_king_side = False
                self.white_can_castle_queen_side = False
            elif piece_moving == "R":
                # if Rook moved away and then moved back, we already made castling that side False
                # and this won't make it True.
                if self.board_array[21] != "R":
                    self.white_can_castle_queen_side = False
                if self.board_array[28] != "R":
                    self.white_can_castle_king_side = False
        if self.black_can_castle_king_side or self.black_can_castle_queen_side:
            if piece_moving == "k":
                self.black_can_castle_king_side = False
                self.black_can_castle_queen_side = False
            elif piece_moving == "r":
                if self.board_array[91] != "r":
                    self.black_can_castle_queen_side = False
                if self.board_array[98] != "r":
                    self.black_can_castle_king_side = False

        if piece_moving == "p" or piece_moving == "P" or move.is_capture:
            self.halfmove_clock = 0
        else:
            self.halfmove_clock += 1

        if self.white_to_move:
            self.white_to_move = False
        else:
            self.white_to_move = True
            self.fullmove_number += 1

    def find_piece(self, piece):
        return self.piece_locations[piece]

    def side_to_move_is_in_check(self):

        if self.white_to_move:
            king_position = self.piece_locations["K"][0]
            enemy_pawn, enemy_bishop, enemy_knight, enemy_queen, enemy_rook, enemy_king = "p", "b", "n", "q", "r", "k"
        else:
            king_position = self.piece_locations["k"][0]
            enemy_pawn, enemy_bishop, enemy_knight, enemy_queen, enemy_rook, enemy_king = "P", "B", "N", "Q", "R", "K"

        for velocity in [-9, -11, 9, 11]:  # look for bishops and queens first
            cur_pos = king_position + velocity
            if self.board_array[cur_pos] == enemy_king:
                return True
            while self.board_array[cur_pos] == " ":
                cur_pos += velocity
            if self.board_array[cur_pos] in [enemy_queen, enemy_bishop]:
                return True

        for velocity in [-10, -1, 1, 10]:  # look for rooks and queens next
            cur_pos = king_position + velocity
            if self.board_array[cur_pos] == enemy_king:
                return True
            while self.board_array[cur_pos] == " ":
                cur_pos += velocity
            if self.board_array[cur_pos] in [enemy_queen, enemy_rook]:
                return True

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for cur_pos in [king_position + 8, king_position - 8, king_position + 12, king_position - 12,
                        king_position + 19, king_position - 19, king_position + 21, king_position - 21]:
            if self.board_array[cur_pos] == enemy_knight:
                return True

        # pawn checks
        if self.white_to_move:
            if self.board_array[king_position + 9] == "p" or self.board_array[king_position + 11] == "p":
                return True
        else:
            if self.board_array[king_position - 9] == "P" or self.board_array[king_position - 11] == "P":
                return True

        return False

    def generate_pinned_piece_list(self):
        # A piece is pinned if there is a piece that would put the current king in check if that piece were removed
        retlist = []

        if self.white_to_move:
            king_position = self.piece_locations["K"][0]
            enemy_bishop, enemy_rook, enemy_queen = "b", "r", "q"
            friendly_piece_list = "P", "N", "B", "R", "Q"
        else:
            king_position = self.piece_locations["k"][0]
            enemy_bishop, enemy_rook, enemy_queen = "B", "R", "Q"
            friendly_piece_list = "p", "n", "b", "r", "q"

        for velocity in [-9, -11, 9, 11]:
            cur_pos = king_position + velocity
            while self.board_array[cur_pos] == " ":
                cur_pos += velocity
            if self.board_array[cur_pos] in friendly_piece_list:
                # now keep going to see if a bishop or queen of the opposite color is the next piece we see
                pinning_pos = cur_pos + velocity
                while self.board_array[pinning_pos] == " ":
                    pinning_pos += velocity
                if self.board_array[pinning_pos] in [enemy_queen, enemy_bishop]:
                    retlist.append(cur_pos)

        for velocity in [-10, -1, 1, 10]:
            cur_pos = king_position + velocity
            while self.board_array[cur_pos] == " ":
                cur_pos += velocity
            if self.board_array[cur_pos] in friendly_piece_list:
                # now keep going to see if a bishop or queen of the opposite color is the next piece we see
                pinning_pos = cur_pos + velocity
                while self.board_array[pinning_pos] == " ":
                    pinning_pos += velocity
                if self.board_array[pinning_pos] in [enemy_queen, enemy_rook]:
                    retlist.append(cur_pos)

        return retlist
