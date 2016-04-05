# File for doing perft tests to validate the move generation logic.

import cProfile
from datetime import datetime
import chessboard
import chessmove_list

global_movecount = []

def calc_moves(board, depth, is_debug=False):
    global global_movecount

    if depth == 0:
        return

    cached_ml = chessboard.GLOBAL_TRANSPOSITION_TABLE.probe(board)
    if cached_ml is None:
        ml = chessmove_list.ChessMoveListGenerator(board)
        ml.generate_move_list()
        chessboard.GLOBAL_TRANSPOSITION_TABLE.insert(board, ml.move_list)
        local_move_list = ml.move_list
    else:
        local_move_list = cached_ml

    global_movecount[depth-1] += len(local_move_list)


    if is_debug:

        for move in local_move_list:
            try:
                before_fen = board.convert_to_fen()
                board.apply_move(move)
                middle_fen = board.convert_to_fen()
            except:
                print("could not apply move %s to board %s" % (chessmove_list.pretty_print_move(move), before_fen))
                print("history:")
                for x in board.move_history:
                    print(chessmove_list.pretty_print_move(x[0]))
                raise

            try:
                calc_moves(board, depth-1)
            except:
                print("previous stack: depth %d applied %s to %s" % (depth, chessmove_list.pretty_print_move(move), before_fen))
                raise

            try:
                board.unapply_move()
                after_fen = board.convert_to_fen()
            except:
                print("could not unapply move %s to board %s" % (chessmove_list.pretty_print_move(move), after_fen))
                raise

            if before_fen != after_fen:
                print("%d : %s : %s : resulted in %s : then rolled back to : %s" % (depth, before_fen, chessmove_list.pretty_print_move(move), middle_fen, after_fen))
                raise AssertionError("Halt")

    else:
        for move in local_move_list:
            board.apply_move(move)
            calc_moves(board, depth-1)
            board.unapply_move()


def perft_test(start_fen, validation_list, flush_cache_between_runs=True, is_debug=False):

    """
    :param start_fen: the FEN that you want to test
    :param validation_list: the expected answers at each depth.
    :param flush_cache_between_runs: True if you are doing performance testing, so board positions aren't held
            between executions of this command.  False if you just want to test for correctness.
    :param is_debug: In debug mode, we do an explicit test of the board position before apply and after
            unapply, enabling this to be used to test that routine as well.  It does slow things down a bit.
    :return: The results are printed to screen.
    """


    global global_movecount
    if flush_cache_between_runs:
        chessboard.GLOBAL_TRANSPOSITION_TABLE = chessboard.ChessPositionCache()
    global_movecount = []
    depth = len(validation_list)
    for i in range(depth):
        global_movecount.append(0)
    board = chessboard.ChessBoard()
    board.load_from_fen(start_fen)

    start_time = datetime.now()
    try:
        calc_moves(board, depth, is_debug=is_debug)
    except:
        print("Failed to complete.  Elapsed time: %s" % (datetime.now() - start_time))
        raise

    end_time = datetime.now()
    print("Completed: %s -- Elapsed time: %s" % (start_fen, end_time-start_time))

    # Note my "depth" variable has depth 0 = the leaves, which is inverted of what people expect.
    # Reversing here for display purposes.
    validation_list.reverse()
    for i in range(depth-1, -1, -1):
        if global_movecount[i] == validation_list[i]:
            print("Ply %d: %d positions" % (depth-i, global_movecount[i]))
        else:
            print("Ply %d *ERROR*: Expected %d positions, got %d." % (depth-i, validation_list[i], global_movecount[i]))

def perft_series():
    # Copied from https://chessprogramming.wikispaces.com/Perft+Results
    perft_test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", [20, 400, 8902, 197281, 4865609])
    perft_test("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", [48, 2039, 97862, 4085603])
    perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", [14, 191, 2812, 43238, 674624, 11030083])
    perft_test("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", [6, 264, 9467, 422333, 15833292])
    perft_test("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", [6, 264, 9467, 422333, 15833292])
    perft_test("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8 ", [44, 1486, 62379, 2103487, 89941194])
    perft_test("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", [46, 2079, 89890, 3894594])

    # Copied from https://github.com/thomasahle/sunfish/blob/master/tests/queen.fen
    perft_test("r1b2rk1/2p2ppp/p7/1p6/3P3q/1BP3bP/PP3QP1/RNB1R1K1 w - - 1 0", [40,1334,50182,1807137])

if __name__ == "__main__":
    # perft_series()
    cProfile.run("perft_series()")

