import chessboard
import chessmove_list
from random import randint


# TO-DO

# Allow computer to play white or black
# Handle stalemate when only two pieces on board are kings
# Handle 50-move stalemate
# Handle repeating position stalemate
# BUG: en passant capture doesn't remove captured piece
# Design a static evaluation function
# Basic multi-ply with static evaluation
# https://chessprogramming.wikispaces.com/Engine+Testing ; https://chessprogramming.wikispaces.com/Perft+Results
# Support for command-line arguments
# Wire in xboard


def process_human_move(board):
    """

    :param board: current position
    :return: True if game continues, False if game ended.  Need to fix this later
    """
    if board.side_to_move_is_in_check():
        print("Check!")
    print(board.pretty_print(True))
    human_move_list = chessmove_list.ChessMoveList(board)
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


def process_computer_move(board):

    if board.side_to_move_is_in_check():
        print("Check!")

    computer_move_list = chessmove_list.ChessMoveList(board)
    computer_move_list.generate_move_list()
    if len(computer_move_list.move_list) == 0:
        print(board.pretty_print("True"))

        if board.side_to_move_is_in_check():
            print("Checkmate! Computer Loses!")
            return False
        else:
            print("Stalemate!")
            return False

    computer_move = computer_move_list.move_list[randint(0, len(computer_move_list.move_list)-1)]
    print(computer_move.pretty_print(True))
    board.apply_move(computer_move)
    return True


def play_game():
    b = chessboard.ChessBoard()
    b.initialize_start_position()
    b.load_from_fen("k5n1/p1p4P/8/1P1P4/8/8/K7/8 w - - 1 1")

    computer_is_black = True
    computer_is_white = False



    done = False

    while not done:
        # White moves first
        if computer_is_white:
            keep_going = process_computer_move(b)
        else:
            keep_going = process_human_move(b)

        if keep_going:
            if computer_is_black:
                keep_going = process_computer_move(b)
            else:
                keep_going = process_human_move(b)

        if not keep_going:
            done = True

play_game()
