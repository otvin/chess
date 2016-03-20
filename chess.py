import chessboard
import chessmove_list
from datetime import datetime
from copy import deepcopy
import argparse
import signal
import sys


# TO-DO

# Handle repeating position stalemate
# opening library
# research into how to program endgames
# transposition tables
# quiescence
# penalize doubled-up pawns


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


def debug_print_movetree_to_file(debug_orig_depth, search_depth, board, move, is_before):
    for i in range(search_depth, debug_orig_depth):
        DEBUGFILE.write("     ")
    DEBUGFILE.write("depth: " + str(search_depth) + " ")
    if is_before:
        DEBUGFILE.write("before ")
    else:
        DEBUGFILE.write("after ")
    if board.white_to_move:
        DEBUGFILE.write("white ")
    else:
        DEBUGFILE.write("black ")
    DEBUGFILE.write(move.pretty_print())
    DEBUGFILE.write(" score is:")
    DEBUGFILE.write(str(board.position_score) + "\n")
    DEBUGFILE.flush()


def alphabeta_recurse(board, search_depth, is_check, alpha, beta, debug_orig_depth=4, debug_to_depth=3):

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
        mybestmove = None
        if board.white_to_move:
            for move in move_list.move_list:
                board.apply_move(move)
                score, opponent_bestmove = alphabeta_recurse(board, search_depth-1, move.is_check, alpha, beta,
                                                             debug_orig_depth)
                if DEBUG:
                    if XBOARD:
                        pass  # to-do - implement "thinking" printouts
                    elif search_depth >= debug_to_depth:
                        debug_print_movetree(debug_orig_depth, search_depth, move, opponent_bestmove, score)
                if score > alpha:
                    alpha = score
                    mybestmove = deepcopy(move)
                board.unapply_move()
                if alpha >= beta:
                    break  # alpha-beta cutoff
            return alpha, mybestmove
        else:

            for move in move_list.move_list:

                board.apply_move(move)
                score, opponent_bestmove = alphabeta_recurse(board, search_depth-1, move.is_check, alpha, beta,
                                                             debug_orig_depth)
                if DEBUG:
                    if XBOARD:
                        pass  # same to-do
                    elif search_depth >= debug_to_depth:
                        debug_print_movetree(debug_orig_depth, search_depth, move, opponent_bestmove, score)
                if score < beta:
                    beta = score
                    mybestmove = deepcopy(move)
                board.unapply_move()

                if beta <= alpha:
                    break  # alpha-beta cutoff
            return beta, mybestmove


def process_computer_move(board, search_depth=3):

    start_time = datetime.now()
    if not XBOARD:
        if board.side_to_move_is_in_check():
            print("Check!")

    computer_move_list = chessmove_list.ChessMoveListGenerator(board)
    computer_move_list.generate_move_list()

    best_score, best_move = alphabeta_recurse(board, search_depth, is_check=False, alpha=-33000, beta=33000,
                                              debug_orig_depth=search_depth, debug_to_depth=search_depth-1)

    assert(best_move is not None)

    end_time = datetime.now()

    if not XBOARD:
        print("Elapsed time: " + str(end_time - start_time))
        print("Move made: ", best_move.pretty_print(True) + " :  Score = " + str(best_score))
        if DEBUG:
            print("Board score:", board.position_score)
            print("Board pieces:", board.piece_count)

    if XBOARD:
        movetext = chessboard.arraypos_to_algebraic(best_move.start)
        movetext += chessboard.arraypos_to_algebraic(best_move.end)
        if best_move.is_promotion:
            movetext += best_move.promoted_to.lower()
        printcommand("move " + movetext)

    board.apply_move(best_move)

    return True


# Required to handle these signals if you want to use xboard

def handle_sigint(signum, frame):
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGINT at " + str(datetime.now()) + "\n")


def handle_sigterm(signum, frame):
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGTERM at " + str(datetime.now()) + "\n")
    sys.exit()


def test_for_end(board):
    move_list = chessmove_list.ChessMoveListGenerator(board)
    move_list.generate_move_list()
    if len(move_list.move_list) == 0:
        # either it's a checkmate or a stalemate
        if board.side_to_move_is_in_check():
            if board.white_to_move:
                printcommand("0-1 {Black mates}")
            else:
                printcommand("1-0 {White mates}")
        else:
            printcommand("1/2-1/2 {Stalemate}")
        return True
    elif (board.piece_count["P"] + board.piece_count["B"] + board.piece_count["N"] + board.piece_count["R"] +
                board.piece_count["Q"] == 0) and (board.piece_count["p"] + board.piece_count["b"] +
                board.piece_count["n"] + board.piece_count["r"] + board.piece_count["q"] == 0):
        # Note: Per xboard documentation, you can do stalemate with KK, KNK, KBK, or KBKB with both B's on same
        # color.  For now, only doing KK.
        printcommand("1/2-1/2 {Stalemate - insufficient material}")
        return True  # king vs. king = draw
    else:
        return False


def print_supported_commands():

    print("Sample move syntax:")
    print("     e2e4  - regular move")
    print("     a7a8q - promotion")
    print("     e1g1  - castle")
    print("")
    print("Other commands:")
    print("")
    print("     debug         - enable debugging output / chessdebug.txt log file")
    print("     force         - human plays both white and black")
    print("     go            - computer takes over for color currently on move")
    print("                   - NOTE: engine will pause between moves if you make computer play both sides")
    print("     help          - this list")
    print("     new           - begin new game, computer black")
    print("     ping TEXT     - reply with 'pong TEXT'")
    print("     quit          - exit game")
    print("     sd N          - set search depth to N plies.  N > 4 will be very slow right now.")
    print("     setboard FEN  - set current position to the FEN that is specified")
    print("     xboard        - use xboard (GNU Chess) protocol")
    print("")
    print("Commands coming soon:")
    print("     undo          - take back a half move")
    print("     remove        - take back a full move")


def printcommand(command):
    print(command)
    if DEBUG and XBOARD and DEBUGFILE is not None:
        DEBUGFILE.write("Command sent: " + command + "\n")
        DEBUGFILE.flush()


def play_game():
    # xboard integration requires us to handle these two signals
    signal.signal(signal.SIGINT, handle_sigint)
    signal.signal(signal.SIGTERM, handle_sigterm)

    # Acknowledgement: Thanks to Sam Tannous - I relied heavily on his implementation of xboard integration
    # in his "shatranj" engine - https://github.com/stannous/shatranj - only putting in xboard because my kids
    # insisted :).

    parser = argparse.ArgumentParser(description="Play chess!")
    parser.add_argument("--debug", help="print debug messages during play", action="store_true", default=False)
    args = parser.parse_args()
    global DEBUG
    DEBUG = args.debug
    global XBOARD
    XBOARD = False
    global DEBUGFILE
    if DEBUG:
        DEBUGFILE = open("chessdebug.txt", "w")
    else:
        DEBUGFILE = None

    b = chessboard.ChessBoard()
    b.initialize_start_position()
    computer_is_black = True
    computer_is_white = False
    search_depth = 3

    done_with_current_game = False
    while True:
        # Check for mate/stalemate
        if not done_with_current_game:
            done_with_current_game = test_for_end(b)

        if DEBUG and XBOARD:
            DEBUGFILE.write("Waiting for command - " + str(datetime.now()) + "\n")
        command = input()
        if DEBUG and XBOARD:
            DEBUGFILE.write("Command received: " + command + "\n")
            DEBUGFILE.flush()

        # xboard documentation can be found at http://home.hccnet.nl/h.g.muller/engine-intf.html
        if command == "xboard" or command[0:8] == "protover":
            XBOARD = True
            printcommand("feature myname=Bejola0.3")
            printcommand("feature ping=1")
            printcommand("feature setboard=1")
            printcommand("feature san=0")
            printcommand("feature sigint=1")
            printcommand("feature sigterm=1")
            printcommand("feature reuse=1")
            printcommand("feature time=0")
            printcommand("feature usermove=0")
            printcommand("feature colors=1")
            printcommand("feature nps=0")
            printcommand("feature debug=1")
            printcommand("feature analyze=0")
            printcommand("feature done=1")
        elif command[0:4] == "ping":
            printcommand("pong " + command[5:])
        elif command == "quit":
            sys.exit()
        elif command == "new":
            b.initialize_start_position()
            computer_is_black = True
            computer_is_white = False
            done_with_current_game = False
        elif command == "debug":
            # This command is only entered by humans, not Xboard.  Xboard must set debug via the command-line flag.
            # This command toggles debug mode on and off.
            if not DEBUG:
                DEBUG = True
                if DEBUGFILE is None:
                    DEBUGFILE = open("chessdebug.txt", "w")
            else:
                DEBUG = False
                DEBUGFILE.close()
                DEBUGFILE = None
        elif command == "force":
            computer_is_black = False
            computer_is_white = False
        elif command == "go":
            if b.white_to_move:
                computer_is_white = True
            else:
                computer_is_black = True
            process_computer_move(b, search_depth)
        elif command[0:8] == "setboard":
            fen = command[9:]
            # To-do - test for legal position, and if illegal position, respond with tellusererror command
            b.load_from_fen(fen)
        elif command == "undo":
            # take back a half move
            b.unapply_move()
        elif command == "remove":
            # take back a full move
            b.unapply_move()
            b.unapply_move()
        elif command[0:2] == "sd":
            search_depth = int(command[3:])
        elif command == "draw":
            # for now, only draw we accept is due to 50-move rule
            # FIDE rule 9.3 - at move 50 without pawn move or capture, either side can claim a draw on their move.
            # Draw is automatic at move 75.  Move 50 = half-move 100.
            if True or b.halfmove_clock >= 100:
                if XBOARD:
                    printcommand("offer draw")
                else:
                    print("Draw claimed under 50-move rule.")
                done_with_current_game = True
            else:
                # for xboard do nothing, just don't accept it
                if not XBOARD:
                    print("Draw invalid - halfmove clock only at: ", b.halfmove_clock)
        elif command[0:6] == "result":
            # game is over, believe due to resignation
            done_with_current_game = True
        elif command == "fen":
            # this is command for terminal, not xboard
            print(b.convert_to_fen())
        elif command == "help":
            # this is a command for terminal, not xboard
            print_supported_commands()
        elif command == "print":
            # this is a command for terminal, not xboard
            print(b.pretty_print(True))
        elif command in ["random", "?", "hint", "hard", "easy", "post", "nopost", "computer"]:
            # treat these as no-ops
            pass
        elif command[0:4] == "name" or command[0:6] == "rating" or command[0:5] == "level"\
                or command[0:8] == "accepted" or command[0:8] == "rejected":
            # no-op
            pass
        else:
            # we assume it is a move
            human_move = None
            try:
                human_move = chessmove_list.return_validated_move(b, command)
            except AssertionError:
                printcommand("Error (unknown command): " + command)
            if human_move is None:
                printcommand("Illegal move: " + command)
            else:
                b.apply_move(human_move)
                if (b.white_to_move and computer_is_white) or (not b.white_to_move and computer_is_black):
                    done_with_current_game = test_for_end(b)
                    if not done_with_current_game:
                        process_computer_move(b, search_depth)

if __name__ == "__main__":
    play_game()
