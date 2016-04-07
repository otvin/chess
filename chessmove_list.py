import chessboard
from random import shuffle

# CONSTANTS for pieces.  7th bit is color
from chessboard import PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, BLACK, WP, BP, WN, BN, WB, BB, WR, BR, WQ, BQ, \
                            WK, BK, EMPTY, OFF_BOARD, W_CASTLE_QUEEN, W_CASTLE_KING, B_CASTLE_QUEEN, \
                            B_CASTLE_KING, W_TO_MOVE, BOARD_IN_CHECK

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

# Positions in the list
START = 0
END = 1
PIECE_MOVING = 2
PIECE_CAPTURED = 3
CAPTURE_DIFFERENTIAL = 4
PROMOTED_TO = 5
MOVE_FLAGS = 6

# Bits in the Flags
MOVE_CASTLE = 1
MOVE_EN_PASSANT = 2
MOVE_CHECK = 4
MOVE_DOUBLE_PAWN = 8

# Special move used to make comparisons fast
NULL_MOVE = [0, 0, 0, 0, 0, 0, 0]


def pretty_print_move(move, is_debug=False, is_san=False):
    start, end, piece_moving, piece_captured, capture_differential, promoted_to, flags = move

    tmpmove = ""

    if move[START] == NULL_MOVE[START]:
        return "{END}"

    if not flags & MOVE_CASTLE:
        if not is_san:
            tmpmove = chessboard.arraypos_to_algebraic(start)
            if not piece_captured:
                tmpmove += "-"
        else:
            if not(piece_moving & PAWN):
                tmpmove = chessboard.piece_to_string_dict[piece_moving].upper()
            elif piece_captured:
                tmpmove = chessboard.arraypos_to_algebraic(start)[0]

        if piece_captured:
            tmpmove += "x"
        tmpmove += chessboard.arraypos_to_algebraic(end)
        if promoted_to:
            tmpmove += " (%s)" % chessboard.piece_to_string_dict[promoted_to]
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
            tmpmove += " %s captured, differential = %d " % (chessboard.piece_to_string_dict[piece_captured],
                                                             capture_differential)
            if flags & MOVE_EN_PASSANT:
                tmpmove += " en passant."
            else:
                tmpmove += "."

    return tmpmove


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
        if move[START] == start_pos and move[END] == end_pos:
            retval = move
            if retval[PROMOTED_TO]:
                if len(algebraic_move) == 4:
                    print("# promotion not provided - assuming queen.")
                    promotion = "q"
                else:
                    promotion = algebraic_move[4]
                if board.board_attributes & W_TO_MOVE:
                    promotion = promotion.upper()
                else:
                    promotion = promotion.lower()
                retval[PROMOTED_TO] = chessboard.string_to_piece_dict[promotion]
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
            outstr += pretty_print_move(move, is_san=True, is_debug=True) + ","
        outstr += "\n"
        return outstr

    def generate_direction_moves(self, start_pos, velocity, perpendicular_velocity, piece_moving):
        """

        :param start_pos:
        :param velocity: -1 would be west, +10 north, +11 northwest, etc
        :param perpendicular_velocity: to save me an if test, the direction 90 degrees from velocity
        :param piece_moving: character representing the piece so we can attach it to the move
        :return: list of move tuples
        """
        ret_list = []
        cur_pos = start_pos + velocity
        if self.board.board_attributes & W_TO_MOVE:
            enemy_list = chessboard.black_piece_list
            enemy_king = BK
        else:
            enemy_list = chessboard.white_piece_list
            enemy_king = WK

        # add all the blank squares in that direction
        while self.board.board_array[cur_pos] == EMPTY:
            ret_list.append([start_pos, cur_pos, piece_moving, 0, 0, 0, 0])
            cur_pos += velocity

        # if first non-blank square is the opposite color, it is a capture
        blocker = self.board.board_array[cur_pos]
        if blocker in enemy_list:
            capture_diff = chessboard.piece_value_dict[blocker] - chessboard.piece_value_dict[piece_moving]
            ret_list.append([start_pos, cur_pos, piece_moving, blocker, capture_diff, 0, 0])

        if piece_moving & QUEEN:
            # Need to look every direction other than back towards where we came from to see if there is a check
            dirlist = [1, -1, 10, -10, 11, -11, 9, -9]
            dirlist.remove(-1 * velocity)
        else:
            # Two ways for the move to be a check.  First, we take the piece that was blocking us from check,
            # so look straight ahead.  Then, look perpendicular.  Cannot put the king into check behind us, else
            # king would have already been in check.
            dirlist = [velocity, perpendicular_velocity, -1 * perpendicular_velocity]

        for move in ret_list:
            for direction in dirlist:
                testpos = move[END] + direction
                while self.board.board_array[testpos] == EMPTY:
                    testpos += direction
                if self.board.board_array[testpos] == enemy_king:
                    move[MOVE_FLAGS] |= MOVE_CHECK
                    break

        return ret_list

    def generate_slide_moves(self, start_pos, piece_moving):

        north_list = self.generate_direction_moves(start_pos, 10, 1, piece_moving)
        west_list = self.generate_direction_moves(start_pos, -1, 10, piece_moving)
        east_list = self.generate_direction_moves(start_pos, 1, 10, piece_moving)
        south_list = self.generate_direction_moves(start_pos, -10, 1, piece_moving)

        return north_list + west_list + east_list + south_list

    def generate_diagonal_moves(self, start_pos, piece_moving):

        nw_list = self.generate_direction_moves(start_pos, 9, 11, piece_moving)
        ne_list = self.generate_direction_moves(start_pos, 11, 9, piece_moving)
        sw_list = self.generate_direction_moves(start_pos, -11, 9, piece_moving)
        se_list = self.generate_direction_moves(start_pos, -9, 11, piece_moving)

        return nw_list + ne_list + sw_list + se_list

    def generate_knight_moves(self, start_pos):
        ret_list = []
        knight = self.board.board_array[start_pos]
        if knight & BLACK:  # quicker than referencing the white_to_move in the board object
            enemy_list = chessboard.white_piece_list
        else:
            enemy_list = chessboard.black_piece_list

        # valid knight moves are +/- 8, 12, 19, and 21 from current position.
        for dest_pos in (start_pos-21, start_pos-19, start_pos-12, start_pos-8,
                         start_pos+21, start_pos+19, start_pos+12, start_pos+8):
            if self.board.board_array[dest_pos] == EMPTY:
                ret_list.append([start_pos, dest_pos, knight, 0, 0, 0, 0])
            else:
                piece = self.board.board_array[dest_pos]
                if piece in enemy_list:
                    capture_diff = chessboard.piece_value_dict[piece] - chessboard.piece_value_dict[knight]
                    ret_list.append([start_pos, dest_pos, knight, piece, capture_diff, 0, 0])

        for move in ret_list:
            pos = move[END]
            for delta in [-21, -19, -12, -8, 21, 19, 12, 8]:
                targetsquare = self.board.board_array[pos+delta]
                if (knight ^ targetsquare) & BLACK and (targetsquare & KING):
                    move[MOVE_FLAGS] |= MOVE_CHECK
                    break

        return ret_list


    def test_for_check_after_castle(self, rook_pos, directions, enemy_king):
        # I could figure directions and enemy_king out from rook_pos, but faster execution to hard code in the caller
        for direction in directions:
            testpos = rook_pos + direction
            while self.board.board_array[testpos] == EMPTY:
                testpos += direction
            if self.board.board_array[testpos] == enemy_king:
                return True
        return False


    def generate_king_moves(self, start_pos, currently_in_check):
        ret_list = []
        king = self.board.board_array[start_pos]
        if king & BLACK:  # quicker than referencing the white_to_move in the board object
            enemy_list = chessboard.white_piece_list
        else:
            enemy_list = chessboard.black_piece_list

        for dest_pos in (start_pos-1, start_pos+9, start_pos+10, start_pos+11,
                         start_pos+1, start_pos-9, start_pos-10, start_pos-11):
            if self.board.board_array[dest_pos] == EMPTY:
                ret_list.append([start_pos, dest_pos, king, 0, 0, 0, 0])
            else:
                piece = self.board.board_array[dest_pos]
                if piece in enemy_list:
                    capture_diff = chessboard.piece_value_dict[piece] - chessboard.piece_value_dict[king]
                    ret_list.append([start_pos, dest_pos, king, piece, capture_diff, 0, 0])

        if not currently_in_check:
            if king == WK and start_pos == 25:
                # arraypos 25 = "e1"
                if self.board.board_attributes & W_CASTLE_KING:
                    if (self.board.board_array[26] == EMPTY and self.board.board_array[27] == EMPTY
                            and self.board.board_array[28] == WR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(26, [-1, 10], BK):
                            flags |= MOVE_CHECK
                        ret_list.append([25, 27, king, 0, 0, 0, flags])
                if self.board.board_attributes & W_CASTLE_QUEEN:
                    if (self.board.board_array[24] == EMPTY and self.board.board_array[23] == EMPTY
                            and self.board.board_array[22] == EMPTY and self.board.board_array[21] == WR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(24, [1, 10], BK):
                            flags |= MOVE_CHECK
                        ret_list.append([25, 23, king, 0, 0, 0, flags])
            elif king == BK and start_pos == 95:
                # arraypos 95 = "e8"
                if self.board.board_attributes & B_CASTLE_KING:
                    if (self.board.board_array[96] == EMPTY and self.board.board_array[97] == EMPTY
                            and self.board.board_array[98] == BR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(96, [-1, -10], WK):
                            flags |= MOVE_CHECK
                        ret_list.append([95, 97, king, 0, 0, 0, flags])
                if self.board.board_attributes & B_CASTLE_QUEEN:
                    if (self.board.board_array[94] == EMPTY and self.board.board_array[93] == EMPTY
                            and self.board.board_array[92] == EMPTY and self.board.board_array[91] == BR):
                        flags = MOVE_CASTLE
                        if self.test_for_check_after_castle(94, [1, -10], WK):
                            flags |= MOVE_CHECK
                        ret_list.append([95, 93, king, 0, 0, 0, flags])

        return ret_list

    def generate_pawn_moves(self, start_pos):
        ret_list = []

        if self.board.board_attributes & W_TO_MOVE:
            pawn, enemypawn = WP, BP
            normal_move, double_move, capture_left, capture_right = (10, 20, 9, 11)
            promotion_list = [WQ, WN, WR, WB]
            enemy_list = chessboard.black_piece_list
            start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max = (31, 38, 81, 88)
        else:
            pawn, enemypawn = BP, WP
            normal_move, double_move, capture_left, capture_right = (-10, -20, -9, -11)
            promotion_list = [BQ, BN, BR, BB]
            enemy_list = chessboard.white_piece_list
            start_rank_min, start_rank_max, penultimate_rank_min, penultimate_rank_max = (81, 88, 31, 38)

        if self.board.board_array[start_pos + normal_move] == EMPTY:
            if penultimate_rank_min <= start_pos <= penultimate_rank_max:
                for promotion in promotion_list:
                    ret_list.append([start_pos, start_pos+normal_move, pawn, 0, 0, promotion, 0])
            else:
                ret_list.append([start_pos, start_pos+normal_move, pawn, 0, 0, 0, 0])
        if ((start_rank_min <= start_pos <= start_rank_max) and
                    self.board.board_array[start_pos + normal_move] == EMPTY and
                    self.board.board_array[start_pos + double_move] == EMPTY):
            ret_list.append([start_pos, start_pos+double_move, pawn, 0, 0, 0, MOVE_DOUBLE_PAWN])

        for dest_pos in [start_pos + capture_left, start_pos + capture_right]:
            dest_square = self.board.board_array[dest_pos]
            if (dest_square in enemy_list or
                        dest_pos == self.board.en_passant_target_square):
                if dest_pos == self.board.en_passant_target_square:
                    ret_list.append([start_pos, dest_pos, pawn, enemypawn, 0, 0, MOVE_EN_PASSANT])
                else:
                    piece_captured = dest_square
                    capture_diff = chessboard.piece_value_dict[piece_captured] - chessboard.piece_value_dict[pawn]
                    if penultimate_rank_min <= start_pos <= penultimate_rank_max:
                        for promotion in promotion_list:
                            ret_list.append([start_pos, dest_pos, pawn, piece_captured, capture_diff, promotion, 0])
                    else:
                        ret_list.append([start_pos, dest_pos, pawn, piece_captured, capture_diff, 0, 0])

        # test moves for check - faster than running side_to_move_in_check later
        for move in ret_list:
            targetleft = self.board.board_array[move[END]+capture_left]
            targetright = self.board.board_array[move[END]+capture_right]
            if (pawn ^ targetleft) & BLACK and targetleft & KING:
                move[MOVE_FLAGS] |= MOVE_CHECK
            elif (pawn ^ targetright) & BLACK and targetright & KING:
                move[MOVE_FLAGS] |= MOVE_CHECK

        for move in ret_list:
            if move[PIECE_CAPTURED] & KING:
                for x in self.board.move_history:
                    print(pretty_print_move(x[0]))
                raise ValueError("CAPTURED KING - preceding move not detected as check or something")

        return ret_list

    def generate_move_list(self, last_best_move=NULL_MOVE, killer_move1 = NULL_MOVE, killer_move2 = NULL_MOVE):
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
        self.move_list = []
        potential_list = []
        capture_list = []
        noncapture_list = []
        check_list = []
        priority_list = []
        killer_list = []

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
            piece_moving = self.board.board_array[move[START]]
            is_king_move = piece_moving & KING
            is_pawn_move = piece_moving & PAWN

            move_valid = True  # assume it is

            self.board.apply_move(move)

            # optimization: only positions where you could move into check are king moves,
            # moves of pinned pieces, or en-passant captures (because could remove two pieces
            # blocking king from check)
            if (currently_in_check or (move[START] in pinned_piece_list) or is_king_move or
                    (move[END] == en_passant_target_square and is_pawn_move)):

                self.board.board_attributes ^= W_TO_MOVE  # apply_moved flipped sides, so flip it back

                if self.board.side_to_move_is_in_check():
                    # if the move would leave the side to move in check, the move is not valid
                    move_valid = False
                elif move[MOVE_FLAGS] & MOVE_CASTLE:
                    # cannot castle through check
                    # kings in all castles move two spaces, so find the place between the start and end,
                    # put the king there, and then test for check again

                    which_king_moving = self.board.board_array[move[END]]
                    which_rook_moving = self.board.board_array[(move[START] + move[END]) // 2]

                    self.board.board_array[(move[START] + move[END]) // 2] = self.board.board_array[move[END]]
                    self.board.piece_locations[which_king_moving][0] = (move[START] + move[END]) // 2

                    self.board.board_array[move[END]] = EMPTY
                    if self.board.side_to_move_is_in_check():
                        move_valid = False

                    # put the king and rook back where they would belong so that unapply move works properly
                    self.board.board_array[(move[START] + move[END]) // 2] = which_rook_moving
                    self.board.board_array[move[END]] = which_king_moving
                    self.board.piece_locations[which_king_moving][0] = move[END]

                self.board.board_attributes ^= W_TO_MOVE  # flip it to the side whose turn it really is

            if move[PIECE_CAPTURED] & KING:
                for x in self.board.move_history:
                    print(pretty_print_move(x[0]))
                raise ValueError("CAPTURED KING - preceding move not detected as check or something")

            if move_valid:
                if (move[START] in discovered_check_list) or move[PROMOTED_TO]:
                    # we tested for all other checks when we generated the move
                    if self.board.side_to_move_is_in_check():
                        move[MOVE_FLAGS] |= MOVE_CHECK
                if last_best_move == move:
                    priority_list = [move]
                elif move[PIECE_CAPTURED]:
                    capture_list += [move]
                elif move == killer_move1 or move == killer_move2:
                    killer_list += [move]
                elif move[MOVE_FLAGS] & MOVE_CHECK:
                    check_list += [move]
                else:
                    noncapture_list += [move]

            self.board.unapply_move()

        shuffle(capture_list)
        capture_list.sort(key=lambda mymove: -mymove[CAPTURE_DIFFERENTIAL])
        shuffle(check_list)
        shuffle(noncapture_list)
        self.move_list = priority_list + capture_list + killer_list + check_list + noncapture_list

