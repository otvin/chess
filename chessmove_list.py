from chessmove import ChessMove
import chessboard

class ChessMoveList:

    def __init__(self, board = None):
        self.move_list = []
        if board is None:
            self.board = chessboard.ChessBoard()
        else:
            self.board = board

    def pretty_print(self):
        outstr = ""
        for move in self.move_list:
            start = chessboard.arraypos_to_algebraic(move.start)
            end = chessboard.arraypos_to_algebraic(move.end)
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

    def generate_direction_moves(self, start_pos, velocity):
        """

        :param start_pos:
        :param velocity: -1 would be west, +10 north, +11 northwest, etc
        :return: list of ChessMoves
        """
        ret_list = []
        cur_pos = start_pos + velocity

        # add all the blank squares in that direction
        while self.board.board_array[cur_pos] == " ":
            ret_list.append(ChessMove(start_pos, cur_pos))
            cur_pos += velocity

        # if first non-blank square is the opposite color, it is a capture
        if self.board.pos_occupied_by_color_not_moving(cur_pos):
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
            if self.board.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos))
            elif self.board.pos_occupied_by_color_not_moving(dest_pos):
                ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))

        return ret_list

    def generate_king_moves(self, start_pos):
        ret_list = []
        for dest_pos in (start_pos-1, start_pos+9, start_pos+10, start_pos+11,
                         start_pos+1, start_pos-9, start_pos-10, start_pos-11):
            if self.board.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos))
            elif self.board.pos_occupied_by_color_not_moving(dest_pos):
                ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))

        if self.board.white_to_move and start_pos == 25:
            # arraypos 25 = "e1"
            if self.board.white_can_castle_king_side:
                if (self.board.board_array[26] == " " and self.board.board_array[27] == " "
                        and self.board.board_array[28] == "R"):
                    ret_list.append(ChessMove(25, 27, is_castle=True))
            if self.board.white_can_castle_queen_side:
                if (self.board.board_array[24] == " " and self.board.board_array[23] == " "
                        and self.board.board_array[22] == " " and self.board.board_array[21] == "R"):
                    ret_list.append(ChessMove(25, 23, is_castle=True))

        if not self.board.white_to_move and start_pos == 95:
            # arraypos 95 = "e8"
            if self.board.black_can_castle_king_side:
                if (self.board.board_array[96] == " " and self.board.board_array[97] == " "
                        and self.board.board_array[98] == "r"):
                    ret_list.append(ChessMove(95, 97, is_castle=True))
            if self.board.black_can_castle_queen_side:
                if (self.board.board_array[94] == " " and self.board.board_array[93] == " "
                        and self.board.board_array[92] == " " and self.board.board_array[91] == "r"):
                    ret_list.append(ChessMove(95, 93, is_castle=True))

        return ret_list

    def generate_pawn_moves(self, start_pos):
        ret_list = []

        if self.board.white_to_move:
            if self.board.board_array[start_pos + 10] == " ":
                if 81 <= start_pos <= 88:  # a7 <= start_pos <= h7
                    for promotion in ["N", "B", "R", "Q"]:
                        ret_list.append(ChessMove(start_pos, start_pos+10, is_promotion=True, promoted_to=promotion))
                else:
                    ret_list.append(ChessMove(start_pos, start_pos+10))
            if (31 <= start_pos <= 38 and self.board.board_array[start_pos + 10] == " "
                    and self.board.board_array[start_pos + 20] == " "):
                ret_list.append(ChessMove(start_pos, start_pos+20, is_two_square_pawn_move=True))
            for dest_pos in [start_pos + 9, start_pos + 11]:
                if (self.board.pos_occupied_by_color_not_moving(dest_pos)
                        or dest_pos == self.board.en_passant_target_square):
                    ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))
        else:
            if self.board.board_array[start_pos - 10] == " ":
                if 31 <= start_pos <= 38:  # a2 <= start_pos <= h2
                    for promotion in ["n", "b", "r", "q"]:
                        ret_list.append(ChessMove(start_pos, start_pos-10, is_promotion=True, promoted_to=promotion))
                else:
                    ret_list.append(ChessMove(start_pos, start_pos-10))
            if (81 <= start_pos <= 88 and self.board.board_array[start_pos - 10] == " "
                    and self.board.board_array[start_pos - 20] == " "):
                ret_list.append(ChessMove(start_pos, start_pos-20, is_two_square_pawn_move=True))
            for dest_pos in [start_pos - 9, start_pos - 11]:
                if (self.board.pos_occupied_by_color_not_moving(dest_pos)
                        or dest_pos == self.board.en_passant_target_square):
                    ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True))

        return ret_list

    def generate_move_list(self):
        self.move_list = []
        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board.board_array[rank + file]
                if piece != " ":
                    if (self.board.white_to_move and piece.isupper()) \
                                or (not self.board.white_to_move and piece.islower()):
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




