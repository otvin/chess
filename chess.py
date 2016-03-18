import chessboard
import chessmove_list
from datetime import datetime
from copy import deepcopy
import argparse


# TO-DO

# Handle stalemate when only two pieces on board are kings
# Handle repeating position stalemate
# Support for command-line arguments
# Wire in xboard
# alpha-beta pruning
# opening library
# research into how to program endgames
# transposition tables
# do move and unmove instead of deepcopies
# quiescence
# penalize doubled-up pawns
# carry the evaluation function around with the board so does not need to be fully recomputed each time.
# when doing that, put the pst's into a dictionary, so pstdict["p"] gets the right piece, so you don't have
#   to have a big if statement like we do below.
# BUG: We are adding/subtracting pieces but we need to subtract/add for Black (reverse it) so Black wants negative score


# initialization of piece-square-tables
# layout of the board - count this way from 0..119

# 110 111 112 113 114 115 116 117 118 119
# 100 101 102 103 104 105 106 107 108 109
# ...
# 10 11 12 13 14 15 16 17 18 19
# 00 01 02 03 04 05 06 07 08 09

# start position is a list based on this (looks mirrored, keep in mind)
#   'xxxxxxxxxx'
#   'xxxxxxxxxx'
#   'xRNBQKBNRx'
#   'xPPPPPPPPx'
#   'x        x'
#   'x        x'
#   'x        x'
#   'x        x'
#   'xppppppppx'
#   'xrnbqkbnrx'
#   'xxxxxxxxxx'
#   'xxxxxxxxxx'

def debug_print_movetree(debug_orig_depth, search_depth, move, opponent_bestmove, score):
    outstr = 5 * " " * (debug_orig_depth-search_depth) + move.pretty_print() + " -> "
    if opponent_bestmove is not None:
        outstr += opponent_bestmove.pretty_print()
    else:
        if score == 0:
            outstr += "[Draw]"
        else:
            outstr += "[Mate]"
    outstr += " " + str(score)
    print(outstr)


def alphabeta_recurse(board, search_depth, is_check, alpha, beta, is_debug=False, debug_orig_depth=4, debug_to_depth=3):

    # Originally I jumped straight to evaluate_board if depth == 0, but that led to very poor evaluation
    # of positions where the position at exactly depth == 0 was a checkmate.  So no matter what, we check
    # for stalemate / checkmate first, and then we decide whether to recurse or statically evaluate.

    move_list = chessmove_list.ChessMoveListGenerator(board)
    move_list.generate_move_list()
    if len(move_list.move_list) == 0:
        if is_check:
            if board.white_to_move:
                return -32000 - search_depth, None  # pick sooner vs. later mates
            else:
                return 32000 + search_depth, None
        else:
            # side cannot move and it is not in check - stalemate
            return 0, None

    if search_depth <= 0:
        return board.evaluate_board(), None
    else:
        cache = chessboard.ChessBoardMemberCache(board)
        mybestmove = None
        if board.white_to_move:
            for move in move_list.move_list:
                board.apply_move(move)
                score, opponent_bestmove = alphabeta_recurse(board, search_depth-1, move.is_check, alpha, beta,
                                                             is_debug, debug_orig_depth)
                if is_debug and search_depth >= debug_to_depth:
                    debug_print_movetree(debug_orig_depth, search_depth, move, opponent_bestmove, score)
                if score > alpha:
                    alpha = score
                    mybestmove = deepcopy(move)
                board.unapply_move(move, cache)
                if alpha >= beta:
                    break  # alpha-beta cutoff
            return alpha, mybestmove
        else:

            for move in move_list.move_list:
                board.apply_move(move)
                score, opponent_bestmove = alphabeta_recurse(board, search_depth-1, move.is_check, alpha, beta,
                                                             is_debug, debug_orig_depth)

                if is_debug and search_depth >= debug_to_depth:
                    debug_print_movetree(debug_orig_depth, search_depth, move, opponent_bestmove, score)
                if score < beta:
                    beta = score
                    mybestmove = deepcopy(move)
                board.unapply_move(move, cache)
                if beta <= alpha:
                    break  # alpha-beta cutoff
            return beta, mybestmove


def process_human_move(board):
    """

    :param board: current position
    :return: True if game continues, False if game ended.  Need to fix this later
    """
    if board.side_to_move_is_in_check():
        print("Check!")
    print(board.pretty_print(True))
    human_move_list = chessmove_list.ChessMoveListGenerator(board)
    human_move_list.generate_move_list()
    if len(human_move_list.move_list) == 0:
        # either it's a checkmate or a stalemate
        if board.side_to_move_is_in_check():
            print("Checkmate! You Lose!")
        else:
            print("Stalemate!")
        return False
    elif (board.piece_count["P"] + board.piece_count["B"] + board.piece_count["N"] + board.piece_count["R"] +
                board.piece_count["Q"] == 0) and (board.piece_count["p"] + board.piece_count["b"] +
                board.piece_count["n"] + board.piece_count["r"] + board.piece_count["q"] == 0):
        print("Stalemate!")
        return False  # king vs. king = draw

    move_is_valid = False
    human_move = None
    while not move_is_valid:
        move_text = input("Enter Move: ")

        # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
        # Draw is automatic at move 75.  Move 50 = half-move 100.
        if move_text.lower() == "draw":
            if board.halfmove_clock >= 100:
                print("Draw claimed under 50-move rule.")
                return False
            else:
                print("Draw invalid - halfmove clock only at: ", board.halfmove_clock)

        if move_text.lower() == "fen":
            print(board.convert_to_fen())

        try:
            human_move = chessmove_list.return_validated_move(board, move_text)
        except AssertionError:
            print("invalid move, try again.")
        else:
            if human_move is None:
                print("invalid move, try again.")
            else:
                if (board.board_array[human_move.start].lower() == "p" and
                        (human_move.end >= 91 or human_move.end <= 29)):
                    promotion_is_valid = False
                    while not promotion_is_valid:
                        promotion = input("Promote to: ").lower()
                        if promotion in ("q", "n", "b", "r"):
                            human_move.promoted_to = promotion
                            promotion_is_valid = True
                            break
                        else:
                            print("invalid promotion, try again.")
                break

    board.apply_move(human_move)
    return True


def process_computer_move(board, search_depth=3, is_debug=False):

    start_time = datetime.now()
    if board.side_to_move_is_in_check():
        print("Check!")

    computer_move_list = chessmove_list.ChessMoveListGenerator(board)
    computer_move_list.generate_move_list()
    if len(computer_move_list.move_list) == 0:
        print(board.pretty_print("True"))

        if board.side_to_move_is_in_check():
            print("Checkmate! Computer Loses!")
            return False
        else:
            print("Stalemate!")
            return False
    elif (board.piece_count["P"] + board.piece_count["B"] + board.piece_count["N"] + board.piece_count["R"] +
                board.piece_count["Q"] == 0) and (board.piece_count["p"] + board.piece_count["b"] +
                board.piece_count["n"] + board.piece_count["r"] + board.piece_count["q"] == 0):
            print("Stalemate!")
            return False  # king vs. king = draw

    best_score, best_move = alphabeta_recurse(board, search_depth, is_check=False, alpha=-33000, beta=33000,
                                              is_debug=is_debug, debug_orig_depth=search_depth,
                                              debug_to_depth=search_depth-1)

    # (board, search_depth, is_check, alpha, beta, is_debug=False, debug_orig_depth=4, debug_to_depth=3)

    assert(best_move is not None)

    end_time = datetime.now()
    print("Elapsed time: " + str(end_time - start_time))
    print("Move made: ", best_move.pretty_print(True) + " :  Score = " + str(best_score))
    if is_debug:
        print("Board score:", board.position_score)
        print("Board pieces:", board.piece_count)
    board.apply_move(best_move)
    return True


def play_game(debug_fen="", is_debug=False, search_depth=3, computer_is_white=False, computer_is_black=False):
    b = chessboard.ChessBoard()
    if debug_fen == "":
        b.initialize_start_position()
    else:
        b.load_from_fen(debug_fen)

    done = False

    while not done:
        if computer_is_white and computer_is_black and b.white_to_move:
            # so people can watch, we print the board every full move
            print(b.pretty_print(True))

        if is_debug:
            print(b.convert_to_fen())
        if b.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            print("Draw due to 75 move rule - FIDE rule 9.3")
            done = True
        elif (b.white_to_move and computer_is_white) or (not b.white_to_move and computer_is_black):
            done = not process_computer_move(b, search_depth=search_depth, is_debug=is_debug)
        else:
            done = not process_human_move(b)

    if is_debug:
        for move in b.move_history:
            print(move.pretty_print())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Play chess!")
    parser.add_argument("-b", "--black", help="computer plays black", action="store_true", default=True)
    parser.add_argument("-w", "--white", help="computer plays white", action="store_true", default=False)
    parser.add_argument("--fen", help="FEN for where game is to start", default="")
    parser.add_argument("--debug", help="print debug messages during play", action="store_true", default=False)
    parser.add_argument("--depth", help="Search depth in plies", default=3, type=int)

    args = parser.parse_args()

    play_game(debug_fen=args.fen, is_debug=args.debug, search_depth=args.depth, computer_is_white=args.white,
              computer_is_black=args.black)
