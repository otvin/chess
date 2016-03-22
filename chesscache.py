import random
from chessboard import ChessBoard


def get_random_board_mask():
    retlist = []
    for i in range(120):
        # this could be 64, but I don't want to have to map a 120-square representation to a 64-square representation
        # when computing a hash
        retlist.append(random.getrandbits(64))
    return retlist

class ChessPositionCache:

    def __init__(self, cachesize=251611):   # 1,299,827 is prime as is 251,611
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

        self.board_mask_dict = {"P": whitep, "p": blackp, "N": whiten, "n": blackn, "B": whiteb, "b": blackb,
                                "R": whiter, "r": blackr, "Q": whiteq, "q": blackq, "K": whitek, "k": blackk}

        self.cachesize = cachesize
        self.cache = [None] * cachesize
        # self.inserts = 0
        # self.probe_hits = 0

    def compute_hash(self, board=ChessBoard()):
        hash = 0
        if board.white_to_move:
            hash = self.whitetomove
        else:
            hash = self.blacktomove

        if board.white_can_castle_king_side:
            hash ^= self.whitecastleking
        if board.white_can_castle_queen_side:
            hash ^= self.whitecastlequeen
        if board.black_can_castle_queen_side:
            hash ^= self.blackcastlequeen
        if board.black_can_castle_king_side:
            hash ^= self.blackcastleking
        if board.en_passant_target_square != -1:
            hash ^= self.enpassanttarget[board.en_passant_target_square]

        for piece in ["p", "n", "b", "r", "q", "k", "P", "N", "B", "R", "Q", "K"]:
            for i in board.piece_locations[piece]:
                hash ^= self.board_mask_dict[piece][i]

        return hash % self.cachesize


    def insert(self, board, stuff):
        # we are using a "replace always" algorithm
        hash = self.compute_hash(board)
        self.cache[hash] = (board.quickstring(), stuff)
        # self.inserts += 1

    def probe(self, board):
        # returns the "stuff" that was cached if board exactly matches what is in the cache.
        # if nothing is in the cache or it is a different board, returns None.
        hash = self.compute_hash(board)
        if self.cache[hash] is None:
            return None
        else:
            c = self.cache[hash]
            if board.quickstring() == c[0]:
                # self.probe_hits += 1
                return c[1]
            else:
                return None
