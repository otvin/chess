import argparse
import signal
import sys
from datetime import datetime
from copy import deepcopy
import chessboard
import chessmove_list
import chesscache

from chessmove_list import START, END, PIECE_MOVING, PIECE_CAPTURED, CAPTURE_DIFFERENTIAL, PROMOTED_TO, MOVE_FLAGS, \
                            MOVE_CASTLE, MOVE_EN_PASSANT, MOVE_CHECK, MOVE_DOUBLE_PAWN, NULL_MOVE

from chessboard import PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, BLACK, WP, BP, WN, BN, WB, BB, WR, BR, WQ, BQ, \
                            WK, BK, EMPTY, OFF_BOARD, W_CASTLE_QUEEN, W_CASTLE_KING, B_CASTLE_QUEEN, \
                            B_CASTLE_KING, W_TO_MOVE, BOARD_IN_CHECK

global_chess_position_move_cache = chesscache.ChessPositionCache()

# global variables
START_TIME = datetime.now()
DEBUG = False
XBOARD = False
POST = False
NODES = 0
DEBUGFILE = None

CACHE_SCORE_EXACT = 0
CACHE_SCORE_HIGH = 1
CACHE_SCORE_LOW = 2

def print_computer_thoughts(orig_search_depth, score, movelist):
    global START_TIME, DEBUG, DEBUGFILE, NODES

    curtime = datetime.now()
    delta = curtime - START_TIME
    centiseconds = (100 * delta.seconds) + (delta.microseconds // 10000)
    movestr = ""
    for move in movelist:
        movestr += chessmove_list.pretty_print_move(move) + " "
    outstr = str(orig_search_depth) + " " + str(score) + " " + str(centiseconds) + " " + str(NODES) + " " + movestr

    if DEBUG:
        DEBUGFILE.write(outstr + "\n")
    print(outstr)


CACHE_HI = 0
CACHE_LOW = 0
CACHE_EXACT = 0

def negamax_recurse(board, depth, alpha, beta, depth_at_root, best_known_line=[]):

    global NODES, DEBUG, POST, global_chess_position_move_cache
    global CACHE_HI, CACHE_LOW, CACHE_EXACT

    # Pseudocode can be found at: https://en.wikipedia.org/wiki/Negamax

    # TO-DO:  Insert "if we have exceeded our maximum time, return, setting some flag that we quit the search.
    # Even though we won't be able to finish the ply, we will have built out the transposition cache.

    NODES += 1
    original_alpha = alpha


    if board.threefold_repetition():
        return 0, [NULL_MOVE]  # Draw - stop searching this position.  Do not cache, as transposition may not always be draw

    cached_position = global_chess_position_move_cache.probe(board)
    if cached_position is not None:
        cached_ml, cache_depth, cache_score, cache_score_type, cached_opponent_movelist = cached_position

        if cache_depth >= depth:
            if cache_score_type == CACHE_SCORE_EXACT:
                CACHE_EXACT += 1
                return cache_score, cached_opponent_movelist
            elif cache_score_type == CACHE_SCORE_LOW:
                if cache_score > alpha:
                    CACHE_LOW += 1
                    alpha = cache_score
                    best_known_line = cached_opponent_movelist
            elif cache_score_type == CACHE_SCORE_HIGH:
                if cache_score < beta:
                    CACHE_HI += 1
                    beta = cache_score
                    best_known_line = cached_opponent_movelist
            if alpha >= beta:
                return cache_score, cached_opponent_movelist

        move_list = cached_ml

        if len(best_known_line) > 0:
            previous_best_move = best_known_line[0]
            for i in range(len(move_list)):
                if move_list[i][START] == previous_best_move[START] and move_list[i][END] == previous_best_move[END]:
                    move_list = [move_list[i]] + move_list[0:i] + move_list[i+1:]

    else:
        if len(best_known_line) > 0:
            previous_best_move = best_known_line[0]
        else:
            previous_best_move = NULL_MOVE
        move_list_generator = chessmove_list.ChessMoveListGenerator(board)
        move_list_generator.generate_move_list(previous_best_move)
        move_list = move_list_generator.move_list

    if len(move_list) == 0:
        if board.board_attributes & BOARD_IN_CHECK:
            retval = -100000 - depth
        else:
            retval = 0
        return retval, [NULL_MOVE]

    if depth <= 0:
        # To-Do: Quiescence.  I had a quiescence search here but it led to weird results.
        # Basic theory of Quiescence is to take the move list, and reduce it significantly and consider
        # only those moves that would occur if the curent state is not stable.

        # For now - just return static evaluation
        if board.threefold_repetition():
            print("FAILURE!")
        return board.evaluate_board(), []

    best_score = -101000
    my_best_move = None


    best_move_sequence = []
    for move in move_list:
        board.apply_move(move)

        # a litle hacky, but cannot use unpacking while also multiplying the score portion by -1
        tmptuple = (negamax_recurse(board, depth-1, -1 * beta, -1 * alpha, depth_at_root, best_known_line[1:]))
        score = -1 * tmptuple[0]
        move_sequence = tmptuple[1]

        board.unapply_move()

        if score > best_score:
            best_score = score
            best_move_sequence = move_sequence
            my_best_move = move
        if score > alpha:
            alpha = score
            if depth == depth_at_root and POST:
                print_computer_thoughts(depth, best_score, [my_best_move] + best_move_sequence)
        if alpha >= beta:
            break  # alpha beta cutoff


    if best_score <= original_alpha:
        cache_score_type = CACHE_SCORE_HIGH
    elif best_score >= beta:
        cache_score_type = CACHE_SCORE_LOW
    else:
        cache_score_type = CACHE_SCORE_EXACT

    global_chess_position_move_cache.insert(board, depth, (move_list, depth, best_score, cache_score_type,
                                                           [my_best_move] + best_move_sequence))

    return best_score, [my_best_move] + best_move_sequence


def process_computer_move(board, best_known_line, search_depth=4, search_time=10000):
    global START_TIME, XBOARD
    global CACHE_HI, CACHE_LOW, CACHE_EXACT, NODES
    global global_chess_position_move_cache

    START_TIME = datetime.now()
    if not XBOARD:
        if board.side_to_move_is_in_check():
            print("Check!")

    CACHE_HI, CACHE_LOW, CACHE_EXACT, NODES = 0, 0, 0, 0

    best_score, best_known_line = negamax_recurse(board, search_depth, -101000, 101000, search_depth, best_known_line)
    print ("NODES:%d CACHE HI:%d  CACHE_LOW:%d  CACHE EXACT:%d" % (NODES, CACHE_HI, CACHE_LOW, CACHE_EXACT))

    assert(len(best_known_line) > 0)

    end_time = datetime.now()
    computer_move = best_known_line[0]

    if not XBOARD:
        print("Elapsed time: " + str(end_time - START_TIME))
        print("Move made: %s : Score = %d" % (chessmove_list.pretty_print_move(computer_move, True), best_score))
        movestr = ""
        for c in best_known_line:
            movestr += chessmove_list.pretty_print_move(c) + " "
        print(movestr)
        if DEBUG:
            print("Board pieces:", board.piece_count)

    if XBOARD:
        movetext = chessboard.arraypos_to_algebraic(computer_move[START])
        movetext += chessboard.arraypos_to_algebraic(computer_move[END])
        if computer_move[PROMOTED_TO]:
            movetext += chessboard.piece_to_string_dict[computer_move[PROMOTED_TO]].lower()
        printcommand("move " + movetext)

    board.apply_move(computer_move)

    # The return value is a tuple of the expected response move, as well as the expected counter to that response.
    # We will use that to seed the iterative deepening in the next round if the opponent makes the move we expect.
    return best_known_line[1:]


# Required to handle these signals if you want to use xboard

def handle_sigint(signum, frame):
    global DEBUGFILE
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGINT at " + str(datetime.now()) + "\n")


def handle_sigterm(signum, frame):
    global DEBUGFILE
    if DEBUGFILE is not None:
        DEBUGFILE.write("got SIGTERM at " + str(datetime.now()) + "\n")
    sys.exit()


def test_for_end(board):

    # Test for draw by repetition:
    fen_count_list = {}
    for move in reversed(board.move_history):
        halfmove_clock, fen = move[3], move[5]
        if halfmove_clock == 0:
            break # no draw by repetition.
        if fen in fen_count_list.keys():
            fen_count_list[fen] += 1
            if fen_count_list[fen] >= 3:
                printcommand("1/2-1/2 {Stalemate - Repetition}")
                return True
        else:
            fen_count_list[fen] = 1

    move_list = chessmove_list.ChessMoveListGenerator(board)
    move_list.generate_move_list()
    if len(move_list.move_list) == 0:
        # either it's a checkmate or a stalemate
        if board.side_to_move_is_in_check():
            if board.board_attributes & W_TO_MOVE:
                printcommand("0-1 {Black mates}")
            else:
                printcommand("1-0 {White mates}")
        else:
            printcommand("1/2-1/2 {Stalemate}")
        return True
    elif (board.piece_count[WP] + board.piece_count[WB] + board.piece_count[WN] + board.piece_count[WR] +
                board.piece_count[WQ] == 0) and (board.piece_count[BP] + board.piece_count[BB] +
                board.piece_count[BN] + board.piece_count[BR] + board.piece_count[BQ] == 0):
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
    print("     both          - computer plays both sides - cannot break until end of game")
    print("     debug         - enable debugging output / chessdebug.txt log file")
    print("     draw          - request draw due to 50 move rule")
    print("     force         - human plays both white and black")
    print("     go            - computer takes over for color currently on move")
    print("                   - NOTE: engine will pause between moves if you make computer play both sides")
    print("     help          - this list")
    print("     history       - print the game's move history")
    print("     new           - begin new game, computer black")
    print("     nopost        - disable POST")
    print("     ping TEXT     - reply with 'pong TEXT'")
    print("     post          - see details on Bejola's thinking")
    print("                   - format: PLY SCORE TIME NODES MOVE_TREE")
    print("                   - where TIME is in centiseconds, and NODES is nodes searched. SCORE < 0 favors black")
    print("     print         - print the board to the terminal")
    print("     printpos      - print a list of pieces and their current positions")
    print("     quit          - exit game")
    print("     remove        - go back a full move")
    print("     resign        - resign your position")
    print("     sd DEPTH      - set search depth to DEPTH plies.  Over 6 will be very slow.  Machine will search")
    print("                   - this depth regardless of time needed.  Default is 4.")
    #  print("     st TIME       - set search time to TIME seconds per move.  Currently, this is a target and is not")
    #  print("                   - strictly enforced, but will be close.  Machine will exceed DEPTH if time remains.")
    #  print("                   - Default is 10 seconds.")
    print("     setboard FEN  - set current position to the FEN that is specified")
    print("     undo          - go back a half move (better: use 'remove' instead)")
    print("     xboard        - use xboard (GNU Chess) protocol")
    print("                   - this command is automatically sent by xboard. Should only")
    print("                   - be used interactively if you want to debug xboard issues.")


def printcommand(command):
    global DEBUG, XBOARD, DEBUGFILE
    print(command)
    if DEBUG and XBOARD and DEBUGFILE is not None:
        DEBUGFILE.write("Command sent: " + command + "\n")
        DEBUGFILE.flush()


def play_game(debugfen=""):
    global DEBUG, XBOARD, POST, DEBUGFILE, global_chess_position_move_cache
    # xboard integration requires us to handle these two signals
    # signal.signal(signal.SIGINT, handle_sigint)
    signal.signal(signal.SIGTERM, handle_sigterm)

    # Acknowledgement: Thanks to Sam Tannous - I relied heavily on his implementation of xboard integration
    # in his "shatranj" engine - https://github.com/stannous/shatranj - only putting in xboard because my kids
    # insisted :).

    parser = argparse.ArgumentParser(description="Play chess with Bejola!")
    parser.add_argument("--debug", help="print debug messages during play", action="store_true", default=False)
    args = parser.parse_args()
    DEBUG = args.debug
    XBOARD = False
    POST = False

    if DEBUG:
        DEBUGFILE = open("chessdebug.txt", "w")
    else:
        DEBUGFILE = None

    b = chessboard.ChessBoard()
    if debugfen != "":
        b.load_from_fen(debugfen)
    else:
        b.initialize_start_position()
    computer_is_black = True
    computer_is_white = False
    search_depth = 4
    search_time = 10000  # milliseconds

    best_known_line = []

    done_with_current_game = False
    while True:
        print("Cache inserts: %d  Cache hits: %d" % (global_chess_position_move_cache.inserts, global_chess_position_move_cache.probe_hits))


        # Check for mate/stalemate
        if not done_with_current_game:
            done_with_current_game = test_for_end(b)

        if computer_is_black and computer_is_white:
            if done_with_current_game:
                computer_is_black = False
                computer_is_white = False
            else:
                best_known_line = process_computer_move(b, best_known_line, search_depth)
                b.required_post_move_updates()
                if not XBOARD:
                    print(b.pretty_print(True))
                best_known_line = process_computer_move(b, best_known_line, search_depth)
                b.required_post_move_updates()
                if not XBOARD:
                    print(b.pretty_print(True))

        else:
            if DEBUG and XBOARD:
                DEBUGFILE.write("Waiting for command - " + str(datetime.now()) + "\n")
            command = input()
            if DEBUG and XBOARD:
                DEBUGFILE.write("Command received: " + command + "\n")
                DEBUGFILE.flush()

            # xboard documentation can be found at http://home.hccnet.nl/h.g.muller/engine-intf.html
            if command == "xboard" or command[0:8] == "protover":
                XBOARD = True
                printcommand('feature myname="Bejola0.5"')
                printcommand("feature ping=1")
                printcommand("feature setboard=1")
                printcommand("feature san=0")
                printcommand("feature sigint=0")
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
            elif command == "both":
                    computer_is_white = True
                    computer_is_black = True
            elif command == "go":
                if b.board_attributes & W_TO_MOVE:
                    computer_is_white = True
                    computer_is_black = False
                else:
                    computer_is_black = True
                    computer_is_white = False
                best_known_line = process_computer_move(b, [], search_depth)
                b.required_post_move_updates()
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
            elif command[0:2] == "st":
                search_time = int(command[3:]) * 1000  # milliseconds
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
            elif command == "history":
                print(b.print_move_history())
            elif command[0:6] == "result" or command[0:6] == "resign":
                # game is over, believe due to resignation
                done_with_current_game = True
            elif command == "post":
                POST = True
            elif command == "nopost":
                POST = False
            elif command == "fen":
                # this is command for terminal, not xboard
                print(b.convert_to_fen())
            elif command == "help":
                # this is a command for terminal, not xboard
                print_supported_commands()
            elif command == "print":
                # this is a command for terminal, not xboard
                print(b.pretty_print(True))
            elif command == "printpos":
                # this is a command for terminal, not xboard
                for piece in b.piece_locations.keys():
                    tmpstr = chessboard.piece_to_string_dict[piece] + ": "
                    if len(b.piece_locations[piece]) == 0:
                        tmpstr += "[None]"
                    else:
                        for loc in b.piece_locations[piece]:
                            tmpstr += chessboard.arraypos_to_algebraic(loc) + " "
                    print(tmpstr)
            elif command in ["random", "?", "hint", "hard", "easy", "computer"]:
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
                    # some bookkeeping done on the board only after real moves, not during best move computation
                    # where we apply/unapply
                    b.required_post_move_updates()
                    if len(best_known_line) > 0:
                        tmpmove = best_known_line[0]
                        if (human_move[START] != tmpmove[START] or
                                human_move[END] != tmpmove[END]):
                            best_known_line = []
                        else:
                            best_known_line = best_known_line[1:]
                    if ((b.board_attributes & W_TO_MOVE) and computer_is_white) or \
                            ((not (b.board_attributes & W_TO_MOVE)) and computer_is_black):
                        done_with_current_game = test_for_end(b)
                        if not done_with_current_game:
                            best_known_line = process_computer_move(b, best_known_line, search_depth)
                            b.required_post_move_updates()

if __name__ == "__main__":
    play_game()
