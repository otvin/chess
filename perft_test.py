# File for doing perft tests to validate the move generation logic.

import chessboard
import chessmove_list
from copy import deepcopy
from datetime import datetime

global_movecount = []


def calc_moves(board, depth):
    if depth == 0:
        return
    ml = chessmove_list.ChessMoveListGenerator(board)
    ml.generate_move_list()
    global_movecount[depth-1] += len(ml.move_list)
    for move in ml.move_list:
        tmpboard = deepcopy(board)
        tmpboard.apply_move(move)
        calc_moves(tmpboard, depth-1)



def perft_test(start_fen, depth):
    for i in range(depth):
        global_movecount.append(0)
    board = chessboard.ChessBoard()
    board.load_from_fen(start_fen)

    start_time = datetime.now()
    calc_moves(board, depth)
    end_time = datetime.now()
    for i in range(depth-1, -1, -1):
        print (global_movecount[i])
    print(end_time-start_time, " elapsed time")


# validation from https://chessprogramming.wikispaces.com/Perft+Results

# testing on the start position
# perft_test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5)
# perft(5) was correct - 4,865,609 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 17 minutes and 47 seconds.
# From reading the internet - a decent game can do perft(6) in under 2 minutes, with some under 3 seconds.  So I am slow.

# position "3" on that page:
# perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5)
# perft(5) was correct - 674,624 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 2 minutes and 45 seconds

# position "4" on that page:
perft_test("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4)
# perft(4) was correct - 422,333 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 1 minute and 37 seconds