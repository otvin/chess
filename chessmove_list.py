from chessmove import ChessMove
import chessboard


def return_validated_move(board, algebraic_move):
    """

    :param board: chessboard with the position in it
    :param algebraic_move: the move in a2-a4 style format
    :return: None if the move is invalid, else the ChessMove object corresponding to the move.
    """

    # out of habit, I type moves e.g. e2-e4, but the xboard protocol uses "e2e4" without the hyphen.
    # I want the game to be tolerant of both.
    if algebraic_move.find("-") == 2:
        algebraic_move = algebraic_move[0:2] + algebraic_move[3:]

    assert(len(algebraic_move) in [4, 5])
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
            outstr += move.pretty_print(is_san=True, is_debug=True) + "\n"
        return outstr

    def generate_direction_moves(self, start_pos, velocity, piece_moving):
        """

        :param start_pos:
        :param velocity: -1 would be west, +10 north, +11 northwest, etc
        :param piece_moving: character representing the piece so we can attach it to the move
        :return: list of ChessMoves
        """
        ret_list = []
        cur_pos = start_pos + velocity
        if self.board.white_to_move:
            enemy_list = chessboard.black_piece_list
        else:
            enemy_list = chessboard.white_piece_list

        # add all the blank squares in that direction
        while self.board.board_array[cur_pos] == " ":
            ret_list.append(ChessMove(start_pos, cur_pos, piece_moving=piece_moving))
            cur_pos += velocity

        # if first non-blank square is the opposite color, it is a capture
        blocker = self.board.board_array[cur_pos]
        if blocker in enemy_list:
            capture_diff = chessboard.piece_value_dict[blocker] - chessboard.piece_value_dict[piece_moving]
            ret_list.append(ChessMove(start_pos, cur_pos, is_capture=True, piece_moving=piece_moving,
                                      piece_captured=blocker, capture_differential=capture_diff))

        return ret_list

    def generate_slide_moves(self, start_pos, piece_moving):

        north_list = self.generate_direction_moves(start_pos, 10, piece_moving)
        west_list = self.generate_direction_moves(start_pos, -1, piece_moving)
        east_list = self.generate_direction_moves(start_pos, 1, piece_moving)
        south_list = self.generate_direction_moves(start_pos, -10, piece_moving)

        return north_list + west_list + east_list + south_list

    def generate_diagonal_moves(self, start_pos, piece_moving):

        nw_list = self.generate_direction_moves(start_pos, 9, piece_moving)
        ne_list = self.generate_direction_moves(start_pos, 11, piece_moving)
        sw_list = self.generate_direction_moves(start_pos, -11, piece_moving)
        se_list = self.generate_direction_moves(start_pos, -9, piece_moving)

        return nw_list + ne_list + sw_list + se_list

    def generate_knight_moves(self, start_pos):
        ret_list = []
        knight = self.board.board_array[start_pos]
        if knight == "N":  # quicker than referencing the white_to_move in the board object
            enemy_list = chessboard.black_piece_list
        else:
            enemy_list = chessboard.white_piece_list

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for dest_pos in (start_pos-21, start_pos-19, start_pos-12, start_pos-8,
                         start_pos+21, start_pos+19, start_pos+12, start_pos+8):
            if self.board.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos, piece_moving=knight))
            else:
                piece = self.board.board_array[dest_pos]
                if piece in enemy_list:
                    capture_diff = chessboard.piece_value_dict[piece] - chessboard.piece_value_dict[knight]
                    ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True, piece_moving=knight,
                                          piece_captured=piece, capture_differential=capture_diff))

        return ret_list

    def generate_king_moves(self, start_pos, currently_in_check):
        ret_list = []
        king = self.board.board_array[start_pos]
        if king == "K":  # quicker than referencing the white_to_move in the board object
            enemy_list = chessboard.black_piece_list
        else:
            enemy_list = chessboard.white_piece_list

        for dest_pos in (start_pos-1, start_pos+9, start_pos+10, start_pos+11,
                         start_pos+1, start_pos-9, start_pos-10, start_pos-11):
            if self.board.board_array[dest_pos] == " ":
                ret_list.append(ChessMove(start_pos, dest_pos, piece_moving=king))
            else:
                piece = self.board.board_array[dest_pos]
                if piece in enemy_list:
                    capture_diff = chessboard.piece_value_dict[piece] - chessboard.piece_value_dict[king]
                    ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True, piece_moving=king,
                                          piece_captured=piece, capture_differential=capture_diff))

        if not currently_in_check:
            if king == "K" and start_pos == 25:
                # arraypos 25 = "e1"
                if self.board.white_can_castle_king_side:
                    if (self.board.board_array[26] == " " and self.board.board_array[27] == " "
                            and self.board.board_array[28] == "R"):
                        ret_list.append(ChessMove(25, 27, is_castle=True, piece_moving=king))
                if self.board.white_can_castle_queen_side:
                    if (self.board.board_array[24] == " " and self.board.board_array[23] == " "
                            and self.board.board_array[22] == " " and self.board.board_array[21] == "R"):
                        ret_list.append(ChessMove(25, 23, is_castle=True, piece_moving=king))

            if not self.board.white_to_move and start_pos == 95:
                # arraypos 95 = "e8"
                if self.board.black_can_castle_king_side:
                    if (self.board.board_array[96] == " " and self.board.board_array[97] == " "
                            and self.board.board_array[98] == "r"):
                        ret_list.append(ChessMove(95, 97, is_castle=True, piece_moving=king))
                if self.board.black_can_castle_queen_side:
                    if (self.board.board_array[94] == " " and self.board.board_array[93] == " "
                            and self.board.board_array[92] == " " and self.board.board_array[91] == "r"):
                        ret_list.append(ChessMove(95, 93, is_castle=True, piece_moving=king))

        return ret_list

    def generate_pawn_moves(self, start_pos):
        ret_list = []

        if self.board.white_to_move:
            pawn, enemypawn = "P", "p"
            normal_move, double_move, capture_left, capture_right = (10, 20, 9, 11)
            promotion_list = ["N", "B", "R", "Q"]
            enemy_list = chessboard.black_piece_list
            start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max = (31, 38, 81, 88)
        else:
            pawn, enemypawn = "p", "P"
            normal_move, double_move, capture_left, capture_right = (-10, -20, -9, -11)
            promotion_list = ["n", "b", "r", "q"]
            enemy_list = chessboard.white_piece_list
            start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max = (81, 88, 31, 38)

        if self.board.board_array[start_pos + normal_move] == " ":
            if penultimate_rank_min <= start_pos <= penultimate_rank_max:
                for promotion in promotion_list:
                    ret_list.append(ChessMove(start_pos, start_pos+normal_move, is_promotion=True,
                                              promoted_to=promotion, piece_moving=pawn))
            else:
                ret_list.append(ChessMove(start_pos, start_pos+normal_move, piece_moving=pawn))
        if ((start_rank_min <= start_pos <= start_rank_max) and
                    self.board.board_array[start_pos + normal_move] == " " and
                    self.board.board_array[start_pos + double_move] == " "):
            ret_list.append(ChessMove(start_pos, start_pos + double_move, is_two_square_pawn_move=True,
                                      piece_moving=pawn))
        for dest_pos in [start_pos + capture_left, start_pos + capture_right]:
            dest_square = self.board.board_array[dest_pos]
            if (dest_square in enemy_list or
                        dest_pos == self.board.en_passant_target_square):
                if dest_pos == self.board.en_passant_target_square:
                        ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True, piece_moving=pawn,
                                                  piece_captured=enemypawn, is_en_passant_capture=True,
                                                  capture_differential=0))
                else:
                    piece_captured = dest_square
                    capture_diff = chessboard.piece_value_dict[piece_captured] - chessboard.piece_value_dict[pawn]
                    if penultimate_rank_min <= start_pos <= penultimate_rank_max:
                        for promotion in promotion_list:
                            ret_list.append(ChessMove(start_pos, dest_pos, is_promotion=True,
                                                  promoted_to=promotion, piece_moving=pawn, is_capture=True,
                                                  piece_captured=piece_captured, capture_differential=capture_diff))
                    else:
                        ret_list.append(ChessMove(start_pos, dest_pos, is_capture=True, piece_moving=pawn,
                                                  piece_captured=piece_captured, capture_differential=capture_diff))

        return ret_list

    def generate_move_list(self, last_best_move=None):
        """

        :param last_best_move: optional - if provided, was the last known best move for this position, and will end up
                first in the return list.
        :return: Updates the move_list member to the moves in order they should be searched.  Current heuristic is
                1) last_best_move goes first if present
                2) Captures go next, in order of MVV-LVA - most valuable victim minus least valuable aggressor
                3) Any moves that put the opponent in check
                4) Any other moves
        """
        self.move_list = []
        potential_list = []
        capture_list = []
        noncapture_list = []
        priority_list = []

        try:
            pinned_piece_list = self.board.generate_pinned_piece_list()
        except:
            print(self.board.pretty_print(True))
            raise

        currently_in_check = self.board.side_to_move_is_in_check()
        en_passant_target_square = self.board.en_passant_target_square

        if self.board.white_to_move:
            pawn, knight, bishop, rook, queen, king = "P", "N", "B", "R", "Q", "K"
        else:
            pawn, knight, bishop, rook, queen, king = "p", "n", "b", "r", "q", "k"

        for piece in self.board.piece_locations[pawn]:
            potential_list += self.generate_pawn_moves(piece)
        for piece in self.board.piece_locations[knight]:
            potential_list += self.generate_knight_moves(piece)
        for piece in self.board.piece_locations[bishop]:
            potential_list += self.generate_diagonal_moves(piece, bishop)
        for piece in self.board.piece_locations[rook]:
            potential_list += self.generate_slide_moves(piece, rook)
        for piece in self.board.piece_locations[queen]:
            potential_list += self.generate_diagonal_moves(piece, queen)
            potential_list += self.generate_slide_moves(piece, queen)
        for piece in self.board.piece_locations[king]:  # could just directly access element [0] but this reads better
            potential_list += self.generate_king_moves(piece, currently_in_check)

        for move in potential_list:
            is_king_move = (self.board.board_array[move.start] in ["K", "k"])
            is_pawn_move = (self.board.board_array[move.start] in ["P", "p"])

            move_valid = True  # assume it is
            self.board.apply_move(move)

            # optimization: only positions where you could move into check are king moves,
            # moves of pinned pieces, or en-passant captures (because could remove two pieces blocking king from check)
            if (currently_in_check or move.start in pinned_piece_list or is_king_move or
                        (move.end == en_passant_target_square and is_pawn_move)):

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
                if move.is_capture:
                    capture_list += [move]
                else:
                    noncapture_list += [move]

            self.board.unapply_move()

        if last_best_move is not None:
            if last_best_move.is_capture:
                for m in capture_list:
                    if m.start == last_best_move.start and m.end == last_best_move.end:
                        priority_list.append(m)
                        capture_list.remove(m)
                        break
            else:
                for m in noncapture_list:
                    if m.start == last_best_move.start and m.end == last_best_move.end:
                        priority_list.append(m)
                        noncapture_list.remove(m)
                        break

        capture_list.sort(key=lambda mymove: -mymove.capture_differential)

        self.move_list =  priority_list + capture_list + noncapture_list
