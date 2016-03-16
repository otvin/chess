from chessboard import arraypos_to_algebraic


class ChessMove:

    def __init__(self, start, end, is_castle=False, is_promotion=False, promoted_to="",
                 is_capture=False, piece_captured="", is_en_passant_capture=False,
                 is_check=False, is_two_square_pawn_move=False):
        self.start = start
        self.end = end
        self.is_castle = is_castle
        self.is_promotion = is_promotion
        self.promoted_to = promoted_to
        self.is_capture = is_capture
        self.is_en_passant_capture = is_en_passant_capture
        self.piece_captured = piece_captured
        self.is_check = is_check
        self.is_two_square_pawn_move = is_two_square_pawn_move

    def pretty_print(self, is_debug=False):

        start = arraypos_to_algebraic(self.start)
        end = arraypos_to_algebraic(self.end)

        if not self.is_castle:
            tmpmove = start
            if self.is_capture:
                tmpmove += "x"
            else:
                tmpmove += "-"
            tmpmove += end
            if self.is_promotion:
                tmpmove += " (" + self.promoted_to + ")"
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
                tmpmove += " " + self.piece_captured + " captured"
                if self.is_en_passant_capture:
                    tmpmove += " en passant."
                else:
                    tmpmove += "."

        return tmpmove
