import chessboard
import chessmove_list
from random import randint


# TO-DO

# Handle stalemate when only two pieces on board are kings
# Handle repeating position stalemate
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

    computer_move = computer_move_list.move_list[randint(0, len(computer_move_list.move_list)-1)]
    print(computer_move.pretty_print(True))
    board.apply_move(computer_move)
    return True


def play_game(is_verbose = False, debug_fen = ""):
    b = chessboard.ChessBoard()
    if debug_fen == "":
        b.initialize_start_position()
    else:
        b.load_from_fen(debug_fen)

    computer_is_black = True
    computer_is_white = False



    done = False

    while not done:
        if is_verbose:
            print(b.convert_to_fen())
        if b.halfmove_clock >= 150:
            # FIDE rule 9.3 - at move 50 without pawn advance or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            print ("Draw due to 75 move rule - FIDE rule 9.3")
            done = True
        elif (b.white_to_move and computer_is_white) or (not b.white_to_move and computer_is_black):
            done = not process_computer_move(b)
        else:
            done = not process_human_move(b)

# play_game(True, "k7/8/pP6/8/8/8/Q7/K7 w - a7 145 102")
play_game(True)