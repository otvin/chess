import random
import chessboard

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
BOARD_IN_CHECK = 32


def get_random_board_mask():
    retlist = []
    for i in range(120):
        # this could be 64, but I don't want to have to map a 120-square representation to a 64-square representation
        # when computing a hash
        retlist.append(random.getrandbits(64))
    return retlist

class ChessPositionCache:

    def __init__(self, cachesize=1048799):   # 1,299,827 is prime as is 251,611
        # random.setstate = ("6052730411110224379233793")  # ensure that we get same cache with every run

        self.whitetomove = random.getrandbits(64)
        self.blacktomove = random.getrandbits(64)
        self.whitecastleking = random.getrandbits(64)
        self.whitecastlequeen = random.getrandbits(64)
        self.blackcastleking = random.getrandbits(64)
        self.blackcastlequeen = random.getrandbits(64)
        self.enpassanttarget = get_random_board_mask()  # could limit this to 16 squares

        whitep = get_random_board_mask()
        blackp = get_random_board_mask()
        whiten = get_random_board_mask()
        blackn = get_random_board_mask()
        whiteb = get_random_board_mask()
        blackb = get_random_board_mask()
        whiter = get_random_board_mask()
        blackr = get_random_board_mask()
        whiteq = get_random_board_mask()
        blackq = get_random_board_mask()
        whitek = get_random_board_mask()
        blackk = get_random_board_mask()

        self.board_mask_dict = {WP: whitep, BP: blackp, WN: whiten, BN: blackn, WB: whiteb, BB: blackb,
                                WR: whiter, BR: blackr, WQ: whiteq, BQ: blackq, WK: whitek, BK: blackk}

        self.cachesize = cachesize
        self.deep_cache = [None] * cachesize
        self.new_cache = [None] * cachesize
        self.deep_inserts = 0
        self.new_inserts = 0
        self.deep_probe_hits = 0
        self.new_probe_hits = 0

    def compute_hash(self, board):
        hash = 0
        if board.board_attributes & W_TO_MOVE:
            hash = self.whitetomove
        else:
            hash = self.blacktomove

        if board.board_attributes & W_CASTLE_KING:
            hash ^= self.whitecastleking
        if board.board_attributes & W_CASTLE_QUEEN:
            hash ^= self.whitecastlequeen
        if board.board_attributes & B_CASTLE_QUEEN:
            hash ^= self.blackcastlequeen
        if board.board_attributes & B_CASTLE_KING:
            hash ^= self.blackcastleking
        if board.en_passant_target_square:
            hash ^= self.enpassanttarget[board.en_passant_target_square]

        for piece in [BP, BN, BB, BR, BQ, BK, WP, WN, WB, WR, WQ, WK]:
            for i in board.piece_locations[piece]:
                hash ^= self.board_mask_dict[piece][i]

        return hash % self.cachesize


    def insert(self, board, depth, stuff):
        # In the depth cache, we store the new record only if the depth is greater than what is there.
        # If it is not greater, then we store it in the new cache, which is in essence the "replace always."

        hash = self.compute_hash(board)
        board.cached_hash = hash

        if self.deep_cache[hash] is None:
            self.deep_cache[hash] = (board.quickstring(), depth, stuff)
            self.deep_inserts += 1
        else:
            if depth >= self.deep_cache[hash][1]:
                self.deep_cache[hash] = (board.quickstring(), depth, stuff)
                self.deep_inserts += 1
            else:
                self.new_cache[hash] = (board.quickstring(), stuff)  # don't waste the bits storing depth here.
                self.new_inserts += 1

    def probe(self, board):
        # Returns the "stuff" that was cached if board exactly matches what is in the cache.
        # Deep cache is checked before new cache, as deep cache should have >= depth than new cache so is
        # more valuable.
        # If nothing is in the cache or it is a different board, returns None.
        hash = self.compute_hash(board)
        board.cached_hash = hash
        if self.deep_cache[hash] is None:
            # deep_cache gets inserted first, so if no deep, then there is no new.
            return None
        else:
            c = self.deep_cache[hash]
            if board.quickstring() == c[0]:
                self.deep_probe_hits += 1
                return c[2]
            else:
                if self.new_cache[hash] is None:
                    return None
                else:
                    c = self.new_cache[hash]
                    if board.quickstring() == c[0]:
                        self.new_probe_hits += 1
                        return c[1]
        return None
