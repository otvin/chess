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

ob = -32767  # short for "off board"

white_pawn_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, 0, 0, 0, 0, 0, 0, 0, 0, ob,
    ob, 5, 10, 10, -20, -20, 10, 10, 5, ob,
    ob, 5, -5, -10, 0, 0, -10, -5, 5, ob,
    ob, 0, 0, 0, 20, 20, 0, 0, 0, ob,
    ob, 5, 5, 10, 25, 25, 10, 5, 5, ob,
    ob, 10, 10, 20, 30, 30, 20, 10, 10, ob,
    ob, 50, 50, 50, 50, 50, 50, 50, 50, ob,
    ob, 0, 0, 0, 0, 0, 0, 0, 0, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

white_knight_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -50, -40, -30, -30, -30, -30, -40, -50, ob,
    ob, -40, -20, 0, 5, 5, 0, -20, -40, ob,
    ob, -30, 5, 10, 15, 15, 10, 5, -30, ob,
    ob, -30, 0, 15, 20, 20, 15, 0, -30, ob,
    ob, -30, 5, 15, 20, 20, 15, 5, -30, ob,
    ob, -30, 0, 10, 15, 15, 10, 0, -30, ob,
    ob, -40, -20, 0, 0, 0, 0, -20, -40, ob,
    ob, -50, -40, -30, -30, -30, -30, -40, -50, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

white_bishop_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -20, -10, -10, -10, -10, -10, -10, -20, ob,
    ob, -10, 5, 0, 0, 0, 0, 5, -10, ob,
    ob, -10, 10, 10, 10, 10, 10, 10, -10, ob,
    ob, -10, 0, 10, 10, 10, 10, 0, -10, ob,
    ob, -10, 5, 5, 10, 10, 5, 5, -10, ob,
    ob, -10, 0, 5, 10, 10, 5, 0, -10, ob,
    ob, -10, 0, 0, 0, 0, 0, 0, -10, ob,
    ob, -20, -10, -10, -10, -10, -10, -10, 20, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

white_rook_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, 0, 0, 0, 5, 5, 0, 0, 0, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, -5, 0, 0, 0, 0, 0, 0, -5, ob,
    ob, 5, 10, 10, 10, 10, 10, 10, 5, ob,
    ob, 0, 0, 0, 0, 0, 0, 0, 0, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

white_queen_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, -20, -10, -10, -5, -5, -10, -10, -20, ob,
    ob, -10, 0, 5, 0, 0, 0, 0, -10, ob,
    ob, -10, 5, 5, 5, 5, 5, 0, -10, ob,
    ob, 0, 0, 5, 5, 5, 5, 0, -5, ob,
    ob, -5, 0, 5, 5, 5, 5, 0, -5, ob,
    ob, -10, 0, 5, 5, 5, 5, 0, -10, ob,
    ob, -10, 0, 0, 0, 0, 0, 0, -10, ob,
    ob, -20, -10, -10, -5, -5, -10, -10, -20, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

# note -the page has a mid and end game, this is mid game, which makes it weak in end game
white_king_pst = [
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, 20, 30, 10, 0, 0, 10, 30, 20, ob,
    ob, 20, 20, 0, 0, 0, 0, 20, 20, ob,
    ob, -10, -20, -20, -20, -20, -20, -20, -10, ob,
    ob, -20, -30, -30, -40, -40, -30, -30, -20, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, -30, -40, -40, -50, -50, -40, -40, -30, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob,
    ob, ob, ob, ob, ob, ob, ob, ob, ob, ob
]

# will initialize these programmatically later
black_pawn_pst = list(120 * " ")
black_knight_pst = list(120 * " ")
black_bishop_pst = list(120 * " ")
black_rook_pst = list(120 * " ")
black_queen_pst = list(120 * " ")
black_king_pst = list(120 * " ")


def debug_print_pst(pst, name):
    outstr = name + "\n"
    for rank in range(90, 10, -10):
        for file in range(1, 9, 1):
            outstr += str(pst[rank + file]) + ", "
        outstr += "\n"
    print(outstr)


def initialize_psts(is_debug=False):
    # Evaluation function stolen from https://chessprogramming.wikispaces.com/Simplified+evaluation+function

    # why am I doing this?  I could have added the value of pieces in the definition
    # and defined the black pst's above instead of doing programatically.  Reason is that
    # this way if I want to change the model slightly, I have to make the change in one place
    # and it will percolate elsewhere automatically, instead of changing potentially dozens
    # of values in multiple lists.

    # add the value of the pieces to the pst's
    pawn_value = 100
    knight_value = 320
    bishop_value = 330
    rook_value = 500
    queen_value = 900
    king_value = 20000

    for rank in range(90, 10, -10):
        for file in range(1, 9, 1):
            white_pawn_pst[rank + file] += pawn_value
            white_knight_pst[rank + file] += knight_value
            white_bishop_pst[rank + file] += bishop_value
            white_rook_pst[rank + file] += rook_value
            white_queen_pst[rank + file] += queen_value
            white_king_pst[rank + file] += king_value

    # to make the black pst's
    # rank 20 maps to rank 90
    # rank 30 maps to rank 80
    # rank 40 maps to rank 70
    # rank 50 maps to rank 60
    # etc.
    # rank 0,10,100,110 are off board

    # initialize off-board ranks
    for file in range(0, 10, 1):
        for rank in [0, 10, 100, 110]:
            black_pawn_pst[rank + file] = ob
            black_knight_pst[rank + file] = ob
            black_bishop_pst[rank + file] = ob
            black_rook_pst[rank + file] = ob
            black_queen_pst[rank + file] = ob
            black_king_pst[rank + file] = ob

    for file in range(0, 10, 1):
        for rankflip in [(20, 90), (30, 80), (40, 70), (50, 60)]:
            black_pawn_pst[rankflip[0] + file] = white_pawn_pst[rankflip[1] + file]
            black_pawn_pst[rankflip[1] + file] = white_pawn_pst[rankflip[0] + file]
            black_knight_pst[rankflip[0] + file] = white_knight_pst[rankflip[1] + file]
            black_knight_pst[rankflip[1] + file] = white_knight_pst[rankflip[0] + file]
            black_bishop_pst[rankflip[0] + file] = white_bishop_pst[rankflip[1] + file]
            black_bishop_pst[rankflip[1] + file] = white_bishop_pst[rankflip[0] + file]
            black_rook_pst[rankflip[0] + file] = white_rook_pst[rankflip[1] + file]
            black_rook_pst[rankflip[1] + file] = white_rook_pst[rankflip[0] + file]
            black_queen_pst[rankflip[0] + file] = white_queen_pst[rankflip[1] + file]
            black_queen_pst[rankflip[1] + file] = white_queen_pst[rankflip[0] + file]
            black_king_pst[rankflip[0] + file] = white_king_pst[rankflip[1] + file]
            black_king_pst[rankflip[1] + file] = white_king_pst[rankflip[0] + file]

    if is_debug:
        debug_print_pst(white_pawn_pst, "White Pawn")
        debug_print_pst(black_pawn_pst, "Black Pawn")
        debug_print_pst(white_knight_pst, "White Knight")
        debug_print_pst(black_knight_pst, "Black Knight")
        debug_print_pst(white_bishop_pst, "White Bishop")
        debug_print_pst(black_bishop_pst, "Black Bishop")
        debug_print_pst(white_rook_pst, "White Rook")
        debug_print_pst(black_rook_pst, "Black Rook")
        debug_print_pst(white_queen_pst, "White Queen")
        debug_print_pst(black_queen_pst, "Black Queen")
        debug_print_pst(white_king_pst, "White King")
        debug_print_pst(black_king_pst, "Black King")


def evaluate_board(board):
    """

    :param board: position to be evaluated

    :return: white score minus black score
    """

    if board.halfmove_clock >= 150:
        # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
        # Draw is automatic at move 75.  Move 50 = half-move 100.
        return 0  # Draw

    white_score = 0
    black_score = 0

    for rank in range(90, 10, -10):
        for file in range(1, 9, 1):
            square = rank + file
            piece = board.board_array[square]
            if piece != " ":  # on sparse boards this results in fewer comparisons below
                if piece == "P":
                    white_score += white_pawn_pst[square]
                elif piece == "p":
                    black_score += black_pawn_pst[square]
                elif piece == "N":
                    white_score += white_knight_pst[square]
                elif piece == "n":
                    black_score += black_knight_pst[square]
                elif piece == "B":
                    white_score += white_bishop_pst[square]
                elif piece == "b":
                    black_score += black_bishop_pst[square]
                elif piece == "R":
                    white_score += white_rook_pst[square]
                elif piece == "r":
                    black_score += black_rook_pst[square]
                elif piece == "Q":
                    white_score += white_queen_pst[square]
                elif piece == "q":
                    black_score += black_queen_pst[square]
                elif piece == "K":
                    white_score += white_king_pst[square]
                elif piece == "k":
                    black_score += black_king_pst[square]

    return white_score - black_score


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
        return evaluate_board(board), None
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

    best_score, best_move = alphabeta_recurse(board, search_depth, is_check=False, alpha=-33000, beta=33000,
                                              is_debug=is_debug, debug_orig_depth=search_depth,
                                              debug_to_depth=search_depth-1)

    # (board, search_depth, is_check, alpha, beta, is_debug=False, debug_orig_depth=4, debug_to_depth=3)

    assert(best_move is not None)

    end_time = datetime.now()
    print("Elapsed time: " + str(end_time - start_time))
    print("Move made: ", best_move.pretty_print(True) + " :  Score = " + str(best_score))
    board.apply_move(best_move)
    return True


def play_game(debug_fen="", is_debug=False, search_depth=3, computer_is_white=False, computer_is_black=False):
    initialize_psts()
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
