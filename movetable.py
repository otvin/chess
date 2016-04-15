# Data structure to try to speed up evaluation of whether or not a position is a check.
# Based on the move tables that GNU Chess uses.

"""
We are going to make an array of board positions, so the positions from 21..28, 31..38, ... , 91..98 are the
interesting ones.  At each cell will be a list of tuples in the following format:

CELL TO CHECK, PIECES IN THAT CELL THAT COULD PUT IN CHECK, WHERE TO GO IN LIST NEXT IF CELL BLOCKED.

If next cell is not blocked, just keep going ahead in the list and interpret the next tuple.  If you get to
None then there is no check.

So let's look at the entry for position 21 (square a1).  A white king in a1 can be in check by a rook or queen on the a file
or first rank, so positions 31,41,51,...,91 or positions 22,23,24,...,28.  King can be in check by a bishop or queen on
the diagonal a1, b2, c3... which is positions 32,43,54,..., 98.  Can be in check by a knight in b3 or c2, so squares
42 or 33, or a pawn in b2 which is position 32.  All the adjacent squares a king could put you in check, which cannot
happen in a game but we want to prevent illegal moves so we test for that too.  So we would have a list that would look
like:

(31, ROOK | QUEEN | KING, 7)   # This is tuple 0
(41, ROOK | QUEEN , 7)
(51, ROOK | QUEEN , 7)
(61, ROOK | QUEEN , 7)
(71, ROOK | QUEEN , 7)
(81, ROOK | QUEEN , 7)
(91, ROOK | QUEEN , 7)
(22, ROOK | QUEEN | KING, 14)  # This is tuple 7
(23, ROOK | QUEEN , 14)
(24, ROOK | QUEEN , 14)
(25, ROOK | QUEEN , 14)
(26, ROOK | QUEEN , 14)
(27, ROOK | QUEEN , 14)
(28, ROOK | QUEEN , 14)
(32, BISHOP | QUEEN | KING | PAWN, 21)  # This is tuple 14
(43, BISHOP | QUEEN, 21)
(54, BISHOP | QUEEN, 21)
(65, BISHOP | QUEEN, 21)
(76, BISHOP | QUEEN, 21)
(87, BISHOP | QUEEN, 21)
(98, BISHOP | QUEEN, 21)
(42, KNIGHT, 22)   # This is Tuple 21
(33, KNIGHT, None) # This is Tuple 22
(None, None, None)

Take first tuple.  Look at the piece in position 31.  If no piece, move to the second tuple.  If there is a piece,
if it's a black Rook, Queen, or King then it is a check so exit.  If it's any other piece, jump ahead to Tuple 7,
because it's blocked.  In the knight list, you move to the next tuple in the list whether square is empty or occupied
by a non-Knight piece, because knights don't get blocked by pieces.  If you end up at None, then you have no check.
Will need two of these tables - one for white and the other for black - because pawns move differently.

"""

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

WHITE_CHECK_TABLE = [list() for l in range(120)]
BLACK_CHECK_TABLE = [list() for l in range(120)]

EMPTY_BOARD = list(
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



def generate_attack_list(start, velocity, pieces, include_pawn):
    retlist = []
    current = start + velocity
    if EMPTY_BOARD[current] != "x":
        # first square is unique because it includes the king, and possibly the pawn
        if include_pawn:
            retlist.append((current, pieces | KING | PAWN))
        else:
            retlist.append((current, pieces | KING))
        current += velocity
        while EMPTY_BOARD[current] != "x":
            retlist.append((current, pieces))
            current += velocity
    return retlist

def generate_knight_list(start):
    retlist = []
    for delta in [-21, -19, -12, -8, 8, 12, 19, 21]:
        if EMPTY_BOARD[start + delta] != "x":
            retlist.append((start + delta, KNIGHT))
    return retlist


def init_check_tables():

    for rank in range (20, 100, 10):
        for file in range (1, 9, 1):
            # Black and White use same slide moves and knight moves
            start = rank + file
            N = generate_attack_list(start, 10, ROOK | QUEEN, False)
            E = generate_attack_list(start, 1, ROOK | QUEEN, False)
            S = generate_attack_list(start, -10, ROOK | QUEEN, False)
            W = generate_attack_list(start, -1, ROOK | QUEEN, False)
            KNIGHTATTACK = generate_knight_list(start)

            # White king is attacked by pawns from the NE, NW; Black King from SE, SW
            WHITE_NE = generate_attack_list(start, 11, BISHOP | QUEEN, True)
            BLACK_NE = generate_attack_list(start, 11, BISHOP | QUEEN, False)
            WHITE_NW = generate_attack_list(start, 9, BISHOP | QUEEN, True)
            BLACK_NW = generate_attack_list(start, 9, BISHOP | QUEEN, False)

            WHITE_SE = generate_attack_list(start, -9, BISHOP | QUEEN, False)
            BLACK_SE = generate_attack_list(start, -9, BISHOP | QUEEN, True)
            WHITE_SW = generate_attack_list(start, -11, BISHOP | QUEEN, False)
            BLACK_SW = generate_attack_list(start, -11, BISHOP | QUEEN, True)

            cur_spot = 0
            white_main_list = []
            black_main_list = []
            for attack_list in [N, E, S, W]:
                # these are same for both colors
                next_spot = cur_spot + len(attack_list)
                for attack in attack_list:
                    white_main_list.append((attack[0], attack[1], next_spot))
                    black_main_list.append((attack[0], attack[1], next_spot))
                    cur_spot += 1

            white_cur_spot = cur_spot
            black_cur_spot = cur_spot

            for attack_list in [WHITE_NE, WHITE_NW, WHITE_SE, WHITE_SW]:
                next_spot = white_cur_spot + len(attack_list)
                for attack in attack_list:
                    white_main_list.append((attack[0], attack[1], next_spot))
                    white_cur_spot += 1

            for attack_list in [BLACK_NE, BLACK_NW, BLACK_SE, BLACK_SW]:
                next_spot = black_cur_spot + len(attack_list)
                for attack in attack_list:
                    black_main_list.append((attack[0], attack[1], next_spot))
                    black_cur_spot += 1

            assert (white_cur_spot == black_cur_spot)

            cur_spot = white_cur_spot
            for attack in KNIGHTATTACK:
                next_spot = cur_spot + 1
                white_main_list.append((attack[0], attack[1], next_spot))
                black_main_list.append((attack[0], attack[1], next_spot))
                cur_spot = cur_spot + 1

            white_main_list.append((None, None, None))
            black_main_list.append((None, None, None))

            WHITE_CHECK_TABLE[start] = white_main_list
            BLACK_CHECK_TABLE[start] = black_main_list

init_check_tables()

def dump_check_tables():
    dumpfile = open("generated_code.pyx","w")
    dumpfile.write("cdef int WHITE_CHECK_TABLE[120][36][3]\n")
    dumpfile.write("cdef int BLACK_CHECK_TABLE[120][36][3]\n")
    dumpfile.write("for i in range(120):\n")
    dumpfile.write("    for j in range(36):\n")
    dumpfile.write("        for k in range(3):\n")
    dumpfile.write("            WHITE_CHECK_TABLE[i][j][k] = 0\n")
    dumpfile.write("            BLACK_CHECK_TABLE[i][j][k] = 0\n")

    for i in range(120):
        for j in range(len(WHITE_CHECK_TABLE[i])):
            for k in range(3):
                if WHITE_CHECK_TABLE[i][j][k] is not None and WHITE_CHECK_TABLE[i][j][k] != 0:
                    dumpfile.write("WHITE_CHECK_TABLE[%d][%d][%d] = %d\n" % (i,j,k,WHITE_CHECK_TABLE[i][j][k]))

    for i in range(120):
        for j in range(len(BLACK_CHECK_TABLE[i])):
            for k in range(3):
                if BLACK_CHECK_TABLE[i][j][k] is not None and BLACK_CHECK_TABLE[i][j][k] != 0:
                    dumpfile.write("BLACK_CHECK_TABLE[%d][%d][%d] = %d\n" % (i,j,k, BLACK_CHECK_TABLE[i][j][k]))


def dump_check_tables_to_c():
    dumpfile = open("check_tables.c","w")
    dumpfile.write("unsigned char WHITE_CHECK_TABLE[120][36][3];\n")
    dumpfile.write("unsigned char BLACK_CHECK_TABLE[120][36][3];\n")
    dumpfile.write("void init_check_tables()\n")
    dumpfile.write("{\n")
    dumpfile.write("    memset(WHITE_CHECK_TABLE, '\\0', sizeof(WHITE_CHECK_TABLE));\n")
    dumpfile.write("    memset(BLACK_CHECK_TABLE, '\\0', sizeof(BLACK_CHECK_TABLE));\n")

    for i in range(120):
        for j in range(len(WHITE_CHECK_TABLE[i])):
            for k in range(3):
                if WHITE_CHECK_TABLE[i][j][k] is not None and WHITE_CHECK_TABLE[i][j][k] != 0:
                    dumpfile.write("    WHITE_CHECK_TABLE[%d][%d][%d] = %d;\n" % (i,j,k,WHITE_CHECK_TABLE[i][j][k]))

    for i in range(120):
        for j in range(len(BLACK_CHECK_TABLE[i])):
            for k in range(3):
                if BLACK_CHECK_TABLE[i][j][k] is not None and BLACK_CHECK_TABLE[i][j][k] != 0:
                    dumpfile.write("    BLACK_CHECK_TABLE[%d][%d][%d] = %d;\n" % (i,j,k, BLACK_CHECK_TABLE[i][j][k]))

    dumpfile.write("}\n")