class ChessMove:

    def __init__(self, start, end, is_castle=False, is_promotion=False, promoted_to="",
                 is_capture=False, is_check=False):
        self.start = start
        self.end = end
        self.is_castle = is_castle
        self.is_promotion = is_promotion
        self.promoted_to = promoted_to
        self.is_capture = is_capture
        self.is_check = is_check

