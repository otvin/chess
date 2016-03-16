# File for doing perft tests to validate the move generation logic.

import chessboard
import chessmove_list
from datetime import datetime

global_movecount = []


def calc_moves(board, depth, is_debug=False):
    if depth == 0:
        return
    ml = chessmove_list.ChessMoveListGenerator(board)
    ml.generate_move_list()
    global_movecount[depth-1] += len(ml.move_list)
    cache = chessboard.ChessBoardMemberCache(board)


    if is_debug:

        for move in ml.move_list:
            try:
                before_fen = board.convert_to_fen()
                board.apply_move(move)
                middle_fen = board.convert_to_fen()
            except:
                print("could not apply move " + move.pretty_print() + " to board " + before_fen)
                print("history:")
                for x in board.move_history:
                    print(x.pretty_print())
                raise

            try:
                calc_moves(board, depth-1)
            except:
                print("previous stack: depth " + str(depth) + ": applied " + move.pretty_print() + " to " + before_fen)
                raise

            try:
                board.unapply_move(move, cache)
                after_fen = board.convert_to_fen()
            except:
                print("could not unapply move " + move.pretty_print() + " to board " + before_fen + ":" + after_fen)
                raise

            if before_fen != after_fen:
                print(str(depth) + " : " + before_fen + " : " + move.pretty_print() + " : resulted in " + middle_fen + " : then rolled back to: " + after_fen)
                raise AssertionError("Halt")

    else:
        for move in ml.move_list:
            board.apply_move(move)
            calc_moves(board, depth-1)
            board.unapply_move(move, cache)


def perft_test(start_fen, depth):
    for i in range(depth):
        global_movecount.append(0)
    board = chessboard.ChessBoard()
    board.load_from_fen(start_fen)

    start_time = datetime.now()
    try:
        calc_moves(board, depth, is_debug=True)
    except:
        print("Failed to complete " + str(datetime.now() - start_time))
        raise

    end_time = datetime.now()
    for i in range(depth-1, -1, -1):
        print(global_movecount[i])
    print(end_time-start_time, " elapsed time")


# validation from https://chessprogramming.wikispaces.com/Perft+Results

# testing on the start position
# perft_test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6)
# perft(5) was correct - 4,865,609 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 17 minutes and 47 seconds.
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in calc_moves - 15:17
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 1:54
# From reading the internet - a decent game can do perft(6) in under 2 minutes, with some under 3 seconds.  So I'm slow.

# position "3" on that page:
perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6)
# perft(5) was correct - 674,624 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 2 minutes and 45 seconds
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in calc_moves - 2:27
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 0:20

# perft(6) was correct - 11,030,083 possibilities
# Historical performance: 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 5:52.

# position "4" on that page:
# perft_test("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4)
# perft(4) was correct - 422,333 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 1 minute and 37 seconds
# 3/15/2016 (v0.1+) - only test for move to be invalid in certain conditions - 1:24
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in calc_moves - 1:14
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 0:11

# perft_test("rn5k/pp6/8/8/8/8/PP6/RN5K w - - 0 1", 5)
