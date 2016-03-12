import chessboard
import chessmove_list
from random import randint


def play_game():
    b = chessboard.ChessBoard()
    b.initialize_start_position()
    done = False

    while not done:
        # For now, human is always white
        if b.side_to_move_is_in_check():
            print("Check!")

        print(b.pretty_print(True))

        # is there a valid move?
        white_list = chessmove_list.ChessMoveList(b)
        white_list.generate_move_list()
        if len(white_list.move_list) == 0:
            # either it's a checkmate or a stalemate
            if b.side_to_move_is_in_check():
                print("Checkmate!  Black Wins!")
                break
            else:
                print("Stalemate!")
                break
        move_is_valid = False
        white_move = None
        while not move_is_valid:
            move_text = input("Enter Move: ")
            try:
                white_move = chessmove_list.return_validated_move(b, move_text)
            except AssertionError:
                print("invalid move, try again.")
            else:
                if white_move is None:
                    print("invalid move, try again.")
                else:
                    break

        b.apply_move(white_move)
        if b.side_to_move_is_in_check():
            print("Check!")

        black_list = chessmove_list.ChessMoveList(b)
        black_list.generate_move_list()
        if len(black_list.move_list) == 0:
            print(b.pretty_print("True"))

            if b.side_to_move_is_in_check():
                print("Checkmate! White Wins!")
                break
            else:
                print("Stalemate!")
                break

        black_move = black_list.move_list[randint(0, len(black_list.move_list)-1)]
        black_move.pretty_print()
        b.apply_move(black_move)


play_game()
