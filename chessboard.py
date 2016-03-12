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
        self.move_list = []

        # To be concise, I would prefer to have init do nothing else, but need to define all members in __init__
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
        self.move_list = []

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
                if piece.isUpper():
                    retval = True
            else:
                if piece.isLower():
                    retval = True
        return retval

    def pos_occupied_by_color_not_moving(self, pos):
        retval = False
        piece = self.board_array[pos]
        if piece != " " and piece != "x":
            if self.white_to_move:
                if piece.islower():
                    retval = True
            else:
                if piece.isupper():
                    retval = True
        return retval

    def apply_move(self, move):
        assert(self.board_array[move.start] != " ")  # make sure start isn't empty
        assert(self.board_array[move.start] != "x")  # make sure start isn't off board
        assert(self.board_array[move.end] != "x")  # make sure end isn't off board

        # this function doesn't validate that the move is legal, just applies the move
        # the asserts are mostly for debugging, may want to remove for performance later.

        piece_moving = self.board_array[move.start]
        self.board_array[move.end] = piece_moving
        self.board_array[move.start] = " "

        self.en_passant_target_square = -1  # will set it below if it needs to be

        if move.is_castle:
            # the move includes the king, need to move the rook
            if move.end == 27:  # white, king side
                assert self.white_can_castle_king_side
                self.board_array[28] = " "
                self.board_array[26] = "R"
                self.white_can_castle_king_side = False
                self.white_can_castle_queen_side = False
            elif move.end == 23:  # white, queen side
                assert self.white_can_castle_queen_side
                self.board_array[21] = " "
                self.board_array[24] = "R"
                self.white_can_castle_king_side = False
                self.white_can_castle_queen_side = False
            elif move.end == 97:  # black, king side
                assert self.black_can_castle_king_side
                self.board_array[98] = " "
                self.board_array[96] = "r"
                self.black_can_castle_king_side = False
                self.black_can_castle_queen_side = False
            elif move.end == 93:  # black, queen side
                assert self.black_can_castle_queen_side
                self.board_array[91] = " "
                self.board_array[94] = "r"
                self.black_can_castle_queen_side = False
                self.black_can_castle_king_side = False
            else:
                raise ValueError("Invalid Castle Move ", move.start, move.end)
        elif move.is_promotion:
            self.board_array[move.end] = move.promoted_to
        elif move.is_two_square_pawn_move:
            if piece_moving == "P":
                self.en_passant_target_square = move.end - 10
            elif piece_moving == "p":
                self.en_passant_target_square = move.end + 10
            else:
                raise ValueError("Invalid 2 square pawn move ", move.start, move.end)

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
        retlist = []
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                if self.board_array[rank+file] == piece:
                    retlist.append(rank+file)
        return retlist

    def side_to_move_is_in_check(self):
        if self.white_to_move:
            king_position = self.find_piece("K")[0]
        else:
            king_position = self.find_piece("k")[0]

        for velocity in [-9, -11, 9, 11]:  # look for bishops and queens first
            cur_pos = king_position + velocity
            if self.board_array[cur_pos] in ["K", "k"]:  # can't be the current king, must be opponent
                return True
            while self.board_array[cur_pos] == " ":
                cur_pos += velocity
            if self.pos_occupied_by_color_not_moving(cur_pos) and self.board_array[cur_pos] in ["Q", "q", "B", "b"]:
                return True

        for velocity in [-10, -1, 1, 10]:  # look for rooks and queens next
            cur_pos = king_position + velocity
            if self.board_array[cur_pos] in ["K", "k"]:
                return True
            while self.board_array[cur_pos] == " ":
                cur_pos += velocity
            if self.pos_occupied_by_color_not_moving(cur_pos) and self.board_array[cur_pos] in ["Q", "q", "R", "r"]:
                return True

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for cur_pos in [king_position + 8, king_position - 8, king_position + 12, king_position - 12,
                        king_position + 19, king_position - 19, king_position + 21, king_position - 21]:
            if self.pos_occupied_by_color_not_moving(cur_pos) and self.board_array[cur_pos] in ["N", "n"]:
                return True

        # pawn checks
        if self.white_to_move:
            if self.board_array[king_position + 9] == "p" or self.board_array[king_position + 11] == "p":
                return True
        else:
            if self.board_array[king_position - 9] == "P" or self.board_array[king_position - 11] == "P":
                return True

        return False

