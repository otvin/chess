import chessboard
import chessmove_list
from datetime import datetime
from copy import deepcopy
import argparse
import signal
import sys


# TO-DO

# Handle repeating position stalemate
# Wire in xboard
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
        cache = chessboard.ChessBoardMemberCache(board)
        mybestmove = None
        if board.white_to_move:
            for move in move_list.move_list:
                board.apply_move(move)
                score, opponent_bestmove = alphabeta_recurse(board, search_depth-1, move.is_check, alpha, beta,
                                                             debug_orig_depth)
                if DEBUG:
                    if XBOARD:
                        pass #to-do - implement "thinking" printouts
                    elif search_depth >= debug_to_depth:
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
                                                             debug_orig_depth)

                if DEBUG:
                    if XBOARD:
                        pass #same to-do
                    elif search_depth >= debug_to_depth:
                        debug_print_movetree(debug_orig_depth, search_depth, move, opponent_bestmove, score)
                if score < beta:
                    beta = score
                    mybestmove = deepcopy(move)
                board.unapply_move(move, cache)
                if beta <= alpha:
                    break  # alpha-beta cutoff
            return beta, mybestmove


def process_human_move(board, humanmove=""):
    """

    :param board: current position
    :return: True if game continues, False if game ended.  Need to fix this later
    """
    if not XBOARD:
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
        elif move_text.lower() == "fen":
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
        print("move " + movetext)

    board.apply_move(best_move)


    return True






# Required to handle these signals if you want to use xboard

def handle_sigint(signum,frame):
    print("gotSIGINT: got signum %s with frame %s" % (signum, frame))
    sys.exit()
    return

def handle_sigterm(signum,frame):
    print("gotSIGTERM: got signum %s with frame %s" % (signum, frame))
    sys.exit()
    return


def test_for_end(board):
    move_list = chessmove_list.ChessMoveListGenerator(board)
    move_list.generate_move_list()
    if len(move_list.move_list) == 0:
        # either it's a checkmate or a stalemate
        if board.side_to_move_is_in_check():
            if board.white_to_move:
                print("0-1 {Black mates}")
            else:
                print("1-0 {White mates}")
        else:
            print("1/2-1/2 {Stalemate}")
        return True
    elif (board.piece_count["P"] + board.piece_count["B"] + board.piece_count["N"] + board.piece_count["R"] +
                board.piece_count["Q"] == 0) and (board.piece_count["p"] + board.piece_count["b"] +
                board.piece_count["n"] + board.piece_count["r"] + board.piece_count["q"] == 0):
        # Note: Per xboard documentation, you can do stalemate with KK, KNK, KBK, or KBKB with both B's on same
        # color.  For now, only doing KK.
        print("1/2-1/2 {Stalemate - insufficient material}")
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

def play_game():
    # xboard integration requires us to handle these two signals
    signal.signal(signal.SIGINT,handle_sigint)
    signal.signal(signal.SIGTERM,handle_sigterm)

    # Acknowledgement: Thanks to Sam Tannous - I relied heavily on his implementation of xboard integration
    # in his "shatranj" engine - https://github.com/stannous/shatranj - only putting in xboard because my kids
    # insisted :).

    parser = argparse.ArgumentParser(description="Play chess!")
    parser.add_argument("--debug", help="print debug messages during play", action="store_true", default=False)
    args = parser.parse_args()
    global DEBUG
    DEBUG = args.debug
    global XBOARD
    XBOARD = True

    b = chessboard.ChessBoard()
    b.initialize_start_position()
    computer_is_black = True
    computer_is_white = False
    search_depth = 3

    if DEBUG:
        debugfile = open("chessdebug.txt","w")
    else:
        debugfile = None

    done = False
    while not done:
        # Check for mate/stalemate
        done = test_for_end(b)

        if not done:
            command = input()
            if DEBUG and XBOARD:
                debugfile.write("Command received: " + command + "\n")

            # xboard documentation can be found at http://home.hccnet.nl/h.g.muller/engine-intf.html
            if command == "xboard" or command[0:8] == "protover":
                XBOARD = True
                print("feature myname='Bejola 0.3'")
                print("feature ping=1")
                print("feature setboard=1")
                print("feature san=0")
                print("feature sigint=1")
                print("feature sigterm=1")
                print("feature reuse=1")
                print("feature time=0")
                print("feature usermove=0")
                print("feature colors=1")
                print("feature nps=0")
                print("feature debug=1")
                print("feature analyze=0")
                print("feature done=1")
            elif (command[0:4] == "ping"):
                print ("pong %s" % (command[5:]))
            elif command == "quit":
                done=True
            elif command == "new":
                b.initialize_start_position()
                computer_is_black = True
                computer_is_white = False
            elif command == "debug":
                # This command is only entered by humans, not Xboard.  Xboard must set debug via the command-line flag.
                # This command toggles debug mode on and off.
                if not DEBUG:
                    DEBUG = True
                    if debugfile is None:
                        debugfile = open("chessdebug.txt","w")
                else:
                    DEBUG = False
                    debugfile.close()
                    debugfile = None
            elif command == "force":
                computer_is_black = False
                computer_is_white = False
            elif command == "go":
                if b.white_to_move:
                    computer_is_white = True
                else:
                    computer_is_black = True
                process_computer_move(b, search_depth)
            elif command[0:8]=="setboard":
                fen = command[9:]
                # To-do - test for legal position, and if illegal position, respond with tellusererror command
                b.load_from_fen(fen)
            elif command=="undo":
                # To-do - need to move the board cache onto the move record in board history so can undo.
                print("tellusererror Undo not supported yet.")
            elif command=="remove":
                # To-do - see "undo" above, same issue
                print("tellusererror Remove not supported yet.")
            elif command[0:2] == "sd":
                search_depth = int(command[3:])
            elif command == "fen":
                # this is command for terminal, not xboard
                print(b.convert_to_fen())
            elif command == "help":
                # this is a command for terminal, not xboard
                print_supported_commands()
            elif command == "print":
                # this is a command for terminal, not xboard
                print(b.pretty_print(True))
            elif command in ["random","?","hint","hard","easy","post","nopost","computer"]:
                # treat these as no-ops
                pass
            elif command[0:4] == "name" or command[0:6] == "rating" or command[0:5] == "level":
                # no-op
                pass
            else:
                # we assume it is a move
                human_move = None
                try:
                    human_move = chessmove_list.return_validated_move(b, command)
                except AssertionError:
                    print("Error (unknown command):", command)
                if human_move is None:
                    print("Illegal move: ",command)
                else:
                    b.apply_move(human_move)
                    if (b.white_to_move and computer_is_white) or (not b.white_to_move and computer_is_black):
                        done = test_for_end(b)
                        if not done:
                            process_computer_move(b, search_depth)


if __name__ == "__main__":
    play_game()




    # parser = argparse.ArgumentParser(description="Play chess!")
    #parser.add_argument("-p", "--players", help="Which side(s) computer plays", choices=["b","w","wb","none"],
    #                   default="b")
    #parser.add_argument("--fen", help="FEN for where game is to start", default="")
    #parser.add_argument("--debug", help="print debug messages during play", action="store_true", default=False)
    #parser.add_argument("--depth", help="Search depth in plies", default=3, type=int)
    #
    #args = parser.parse_args()

    #play_game(debug_fen=args.fen, is_debug=args.debug, search_depth=args.depth,
    #          computer_is_white=(args.players in ["w","wb"]), computer_is_black=(args.players in ["b","wb"]))
