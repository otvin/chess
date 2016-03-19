from chessmove import ChessMove
import chessboard


def return_validated_move(board, algebraic_move):
    """

    :param board: chessboard with the position in it
    :param algebraic_move: the move in a2-a4 style format
    :return: None if the move is invalid, else the ChessMove object corresponding to the move.
    """

    assert(len(algebraic_move) in [4,5])
    assert(algebraic_move[0] in ["a", "b", "c", "d", "e", "f", "g", "h"])
    assert(algebraic_move[2] in ["a", "b", "c", "d", "e", "f", "g", "h"])
    assert(algebraic_move[1] in ["1", "2", "3", "4", "5", "6", "7", "8"])
    assert(algebraic_move[3] in ["1", "2", "3", "4", "5", "6", "7", "8"])

    start_pos = chessboard.algebraic_to_arraypos(algebraic_move[0:2])
    end_pos = chessboard.algebraic_to_arraypos(algebraic_move[2:4])
    move_list = ChessMoveListGenerator(board)
    retval = None

    move_list.generate_move_list()
    for move in move_list.move_list:
        if move.start == start_pos and move.end == end_pos:
            retval = move
            if retval.is_promotion:
                if len(algebraic_move) == 4:
                    print("# promotion not provided - assuming queen.")
                    promotion = "q"
                else:
                    promotion = algebraic_move[4]
                if board.white_to_move:
                    promotion = promotion.upper()
                else:
                    promotion = promotion.lower()
                retval.promoted_to = promotion
            break
    return retval


class ChessMoveListGenerator:

    def __init__(self, board=None):
        self.move_list = []
        if board is None:
            self.board = chessboard.ChessBoard()
        else:
            self.board = board

    def pretty_print(self):
        outstr = ""
        for move in self.move_list:
            outstr += move.pretty_print() + "\n"
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
            ret_list.append(ChessMove(start_pos, cur_pos, is_capture=True,
                                      piece_captured=self.board.board_array[cur_pos]))

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
                ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True,
                                          piece_captured=self.board.board_array[dest_pos]))

        return ret_list

    def generate_king_moves(self, start_pos):
        ret_list = []
        for dest_pos in (start_pos-1, start_pos+9, start_pos+10, start_pos+11,
                         start_pos+1, start_pos-9, start_pos-10, start_pos-11):
            if self.board.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos))
            elif self.board.pos_occupied_by_color_not_moving(dest_pos):
                ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True,
                                          piece_captured=self.board.board_array[dest_pos]))

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
                    if 91 <= dest_pos <= 98:
                        for promotion in ["N", "B", "R", "Q"]:
                            ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True, is_promotion=True,
                                            promoted_to=promotion, piece_captured=self.board.board_array[dest_pos]))
                    else:
                        if dest_pos == self.board.en_passant_target_square:
                            # en passant
                            ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True,
                                                      piece_captured=self.board.board_array[dest_pos-10],
                                                      is_en_passant_capture=True))
                        else:
                            ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True,
                                              piece_captured=self.board.board_array[dest_pos]))

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
                    if 21 <= dest_pos <= 28:
                        for promotion in ["n", "b", "r", "q"]:
                            ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True, is_promotion=True,
                                            promoted_to=promotion, piece_captured=self.board.board_array[dest_pos]))
                    else:
                        if dest_pos == self.board.en_passant_target_square:
                            # en passant
                            ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True,
                                                      piece_captured=self.board.board_array[dest_pos+10],
                                                      is_en_passant_capture=True))
                        else:

                            ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True,
                                                    piece_captured=self.board.board_array[dest_pos]))

        return ret_list

    def generate_move_list(self):
        self.move_list = []
        potential_list = []

        try:
            pinned_piece_list = self.board.generate_pinned_piece_list()
        except:
            print(self.board.pretty_print(True))
            raise

        currently_in_check = self.board.side_to_move_is_in_check()

        for rank in range(20, 100, 10):
            for file in range(1, 9, 1):
                piece = self.board.board_array[rank + file]
                if piece != " ":
                    if (self.board.white_to_move and piece.isupper()) \
                                or (not self.board.white_to_move and piece.islower()):
                        if piece == "P" or piece == "p":
                            potential_list += self.generate_pawn_moves(rank + file)
                        elif piece == "N" or piece == "n":
                            potential_list += self.generate_knight_moves(rank + file)
                        elif piece == "B" or piece == "b":
                            potential_list += self.generate_diagonal_moves(rank + file)
                        elif piece == "R" or piece == "r":
                            potential_list += self.generate_slide_moves(rank + file)
                        elif piece == "Q" or piece == "q":
                            potential_list += self.generate_slide_moves(rank + file)
                            potential_list += self.generate_diagonal_moves(rank + file)
                        elif piece == "K" or piece == "k":
                            potential_list += self.generate_king_moves(rank + file)

        cache = chessboard.ChessBoardMemberCache(self.board)

        for move in potential_list:
            is_king_move = (self.board.board_array[move.start] in ["K", "k"])
            is_pawn_move = (self.board.board_array[move.start] in ["P", "p"])

            move_valid = True  # assume it is
            self.board.apply_move(move)

            # optimization: only positions where you could move into check are king moves,
            # moves of pinned pieces, or en-passant captures (because could remove two pieces blocking king from check)
            if (currently_in_check or move.start in pinned_piece_list or is_king_move or
                        (move.end == cache.en_passant_target_square and is_pawn_move)):

                self.board.white_to_move = not self.board.white_to_move  # apply_moved flipped sides, so flip it back

                if self.board.side_to_move_is_in_check():
                    # if the move would leave the side to move in check, the move is not valid
                    move_valid = False
                elif move.is_castle:
                    # cannot castle through check
                    # kings in all castles move two spaces, so find the place between the start and end,
                    # put the king there, and then test for check again

                    which_king_moving = self.board.board_array[move.end]
                    which_rook_moving = self.board.board_array[(move.start + move.end) // 2]

                    self.board.board_array[(move.start + move.end) // 2] = self.board.board_array[move.end]
                    self.board.board_array[move.end] = " "
                    if self.board.side_to_move_is_in_check():
                        move_valid = False

                    # put the king and rook back where they would belong so that unapply move works properly
                    self.board.board_array[(move.start + move.end) // 2] = which_rook_moving
                    self.board.board_array[move.end] = which_king_moving

                self.board.white_to_move = not self.board.white_to_move  # flip it to the side whose turn it really is

            if move_valid:
                if self.board.side_to_move_is_in_check():
                    move.is_check = True
                self.move_list += [move]

            self.board.unapply_move(move, cache)
