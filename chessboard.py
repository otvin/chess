from chessmove import ChessMove

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

    def pretty_print(self):
        outstr = ""
        for i in range(90, 10, -10):
            for j in range(1, 9, 1):
                outstr += self.board_array[i+j]
            outstr += "\n"
        return outstr

    def pretty_print_movelist(self):
        outstr = ""
        for move in self.move_list:
            start = arraypos_to_algebraic(move.start)
            end = arraypos_to_algebraic(move.end)
            tmpmove = ""
            if not move.is_castle:
                tmpmove = start
                if move.is_capture:
                    tmpmove += "x"
                else:
                    tmpmove += "-"
                tmpmove += end
                if move.is_promotion:
                    tmpmove += " (" + move.promoted_to + ")"
            else:
                if end > start:
                    tmpmove = "O-O"
                else:
                    tmpmove = "O-O-O"

            if move.is_check:
                tmpmove += "+"

            outstr += tmpmove + "\n"
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

    def generate_direction_moves(self, start_pos, velocity):
        """

        :param start_pos:
        :param velocity: -1 would be west, +10 north, +11 northwest, etc
        :return: list of ChessMoves
        """
        ret_list = []
        cur_pos = start_pos + velocity

        # add all the blank squares in that direction
        while self.board_array[cur_pos] == " ":
            ret_list.append(ChessMove(start_pos, cur_pos))
            cur_pos += velocity

        # if first non-blank square is the opposite color, it is a capture
        if self.pos_occupied_by_color_not_moving(cur_pos):
            ret_list.append(ChessMove(start_pos, cur_pos, is_capture=True))

        return ret_list

    def generate_slide_moves(self, start_pos):

        north_list = self.generate_direction_moves(start_pos, 10)
        west_list = self.generate_direction_moves(start_pos, -1)
        east_list = self.generate_direction_moves(start_pos, 1)
        south_list = self.generate_direction_moves(start_pos, -10)

        return north_list + west_list + east_list + south_list

    def generate_diagonal_moves(self, start_pos):

        nw_list = self.generate_direction_moves(start_pos, 9)
        ne_list = self.generate_direction_moves(start_pos, 11)
        sw_list = self.generate_direction_moves(start_pos, -11)
        se_list = self.generate_direction_moves(start_pos, -9)

        return nw_list + ne_list + sw_list + se_list

    def generate_knight_moves(self, start_pos):
        ret_list = []
        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for dest_pos in (start_pos-21, start_pos-19, start_pos-12, start_pos-8,
                         start_pos+21, start_pos+19, start_pos+12, start_pos+8):
            if self.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos))
            elif self.pos_occupied_by_color_not_moving(dest_pos):
                ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))

        return ret_list

    def generate_king_moves(self, start_pos):
        ret_list = []
        for dest_pos in (start_pos-1, start_pos+9, start_pos+10, start_pos+11,
                         start_pos+1, start_pos-9, start_pos-10, start_pos-11):
            if self.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos))
            elif self.pos_occupied_by_color_not_moving(dest_pos):
                ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))

        if self.white_to_move and start_pos == 25:
            # arraypos 25 = "e1"
            if self.white_can_castle_king_side:
                if self.board_array[26] == " " and self.board_array[27] == " " and self.board_array[28] == "R":
                    ret_list.append(ChessMove(25, 27, is_castle=True))
            if self.white_can_castle_queen_side:
                if (self.board_array[24] == " " and self.board_array[23] == " " and self.board_array[22] == " "
                        and self.board_array[21] == "R"):
                    ret_list.append(ChessMove(25, 23, is_castle=True))

        if not self.white_to_move and start_pos == 95:
            # arraypos 95 = "e8"
            if self.black_can_castle_king_side:
                if self.board_array[96] == " " and self.board_array[97] == " " and self.board_array[98] == "r":
                    ret_list.append(ChessMove(95, 97, is_castle=True))
            if self.black_can_castle_queen_side:
                if (self.board_array[94] == " " and self.board_array[93] == " " and self.board_array[92] == " "
                        and self.board_array[91] == "r"):
                    ret_list.append(ChessMove(95, 93, is_castle=True))

        return ret_list

    def generate_pawn_moves(self, start_pos):
        ret_list = []

        if self.white_to_move:
            if self.board_array[start_pos + 10] == " ":
                if 81 <= start_pos <= 88:  # a7 <= start_pos <= h7
                    for promotion in ["N", "B", "R", "Q"]:
                        ret_list.append(ChessMove(start_pos, start_pos+10, is_promotion=True, promoted_to=promotion))
                else:
                    ret_list.append(ChessMove(start_pos, start_pos+10))
            if (31 <= start_pos <= 38 and self.board_array[start_pos + 10] == " "
                    and self.board_array[start_pos + 20] == " "):
                ret_list.append(ChessMove(start_pos, start_pos+20, is_two_square_pawn_move=True))
            for dest_pos in [start_pos + 9, start_pos + 11]:
                if self.pos_occupied_by_color_not_moving(dest_pos) or dest_pos == self.en_passant_target_square:
                    ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))
        else:
            if self.board_array[start_pos - 10] == " ":
                if 31 <= start_pos <= 38:  # a2 <= start_pos <= h2
                    for promotion in ["n", "b", "r", "q"]:
                        ret_list.append(ChessMove(start_pos, start_pos-10, is_promotion=True, promoted_to=promotion))
                else:
                    ret_list.append(ChessMove(start_pos, start_pos-10))
            if (81 <= start_pos <= 88 and self.board_array[start_pos - 10] == " "
                    and self.board_array[start_pos - 20] == " "):
                ret_list.append(ChessMove(start_pos, start_pos-20, is_two_square_pawn_move=True))
            for dest_pos in [start_pos - 9, start_pos - 11]:
                if self.pos_occupied_by_color_not_moving(dest_pos) or dest_pos == self.en_passant_target_square:
                    ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))

        return ret_list

    def generate_move_list(self):
        self.move_list = []
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board_array[rank + file]
                if piece != " ":
                    if (self.white_to_move and piece.isupper()) or (not self.white_to_move and piece.islower()):
                        if piece == "P" or piece == "p":
                            self.move_list += self.generate_pawn_moves(rank + file)
                        elif piece == "N" or piece == "n":
                            self.move_list += self.generate_knight_moves(rank + file)
                        elif piece == "B" or piece == "b":
                            self.move_list += self.generate_diagonal_moves(rank + file)
                        elif piece == "R" or piece == "r":
                            self.move_list += self.generate_slide_moves(rank + file)
                        elif piece == "Q" or piece == "q":
                            self.move_list += self.generate_slide_moves(rank + file)
                            self.move_list += self.generate_diagonal_moves(rank + file)
                        elif piece == "K" or piece == "k":
                            self.move_list += self.generate_king_moves(rank + file)
