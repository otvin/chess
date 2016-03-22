from chessboard import arraypos_to_algebraic, piece_to_string_dict


# These are copied from chessboard.py for speed to save the lookup to that module.  While horrible style, I could put
# everything in a single module and everything would be one big long file, and I would only need to declare the
# constants once.  This keeps things modular, and I will forgive myself this sin.

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


class ChessMove:

    def __init__(self, start, end, is_castle=False, is_promotion=False, promoted_to=EMPTY,
                 is_capture=False, piece_captured=EMPTY, is_en_passant_capture=False,
                 is_check=False, is_two_square_pawn_move=False, piece_moving=EMPTY, capture_differential=0):
        self.start = start
        self.end = end
        self.is_castle = is_castle
        self.is_promotion = is_promotion
        self.promoted_to = promoted_to
        self.is_capture = is_capture
        self.is_en_passant_capture = is_en_passant_capture
        self.piece_captured = piece_captured
        self.capture_differential = capture_differential
        self.is_check = is_check
        self.is_two_square_pawn_move = is_two_square_pawn_move
        self.piece_moving = piece_moving

    def pretty_print(self, is_debug=False, is_san=False):

        start = arraypos_to_algebraic(self.start)
        end = arraypos_to_algebraic(self.end)
        tmpmove = ""

        if not self.is_castle:
            if not is_san:
                tmpmove = start
                if not self.is_capture:
                    tmpmove += "-"
            else:
                if not(self.piece_moving & PAWN):
                    tmpmove = piece_to_string_dict[self.piece_moving].upper()
                elif self.is_capture:
                    tmpmove = start[0]

            if self.is_capture:
                tmpmove += "x"
            tmpmove += end
            if self.is_promotion:
                tmpmove += " (" + piece_to_string_dict[self.promoted_to] + ")"
        else:
            if end > start:
                tmpmove = "O-O"
            else:
                tmpmove = "O-O-O"

        if self.is_check:
            tmpmove += "+"

        if is_debug:
            if self.is_two_square_pawn_move:
                tmpmove += " 2 square pawn move"
            if self.is_capture:
                tmpmove += " " + piece_to_string_dict[self.piece_captured] + " captured, differential = " + str(self.capture_differential)
                if self.is_en_passant_capture:
                    tmpmove += " en passant."
                else:
                    tmpmove += "."

        return tmpmove
