import argparse
import signal
import sys
from datetime import datetime
from copy import deepcopy
import chessboard
import chessmove_list
import chesscache

from chessmove_list import START, END, PIECE_MOVING, PIECE_CAPTURED, CAPTURE_DIFFERENTIAL, PROMOTED_TO, MOVE_FLAGS, \
                            MOVE_CASTLE, MOVE_EN_PASSANT, MOVE_CHECK, MOVE_DOUBLE_PAWN

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
CACHE_SCORE_IGNORE = 3

def debug_print_movetree(orig_search_depth, current_search_depth, move, opponent_bestmove_list, score):
    outstr = 5 * " " * (orig_search_depth-current_search_depth) + chessmove_list.pretty_print_move(move) + " -> "
    if opponent_bestmove_list is not None:
        if len(opponent_bestmove_list) >= 1:
            if opponent_bestmove_list[0] is not None:
                outstr += chessmove_list.pretty_print_move(opponent_bestmove_list[0])
            else:
                outstr += "[NONE]"
    else:
        if score == 0:
            outstr += "[Draw]"
        else:
            outstr += "[Mate]"
    outstr += " " + str(score)
    print(outstr)


def debug_print_movetree_to_file(orig_search_depth, current_search_depth, board, move, is_before):
    global DEBUGFILE

    for i in range(current_search_depth, orig_search_depth):
        DEBUGFILE.write("     ")
    DEBUGFILE.write("depth: " + str(current_search_depth) + " ")
    if is_before:
        DEBUGFILE.write("before ")
    else:
        DEBUGFILE.write("after ")
    if board.white_to_move:
        DEBUGFILE.write("white ")
    else:
        DEBUGFILE.write("black ")
    DEBUGFILE.write(chessmove_list.pretty_print_move(move))
    DEBUGFILE.write(" score is:")
    DEBUGFILE.flush()


def print_computer_thoughts(orig_search_depth, score, movelist):
    global START_TIME, DEBUG, DEBUGFILE

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


HIT_LOW = 0
HIT_HIGH = 0
HIT_EXACT = 0
HIT_IGNORE = 0
HIT_LOWQ = 0
HIT_HIGHQ = 0
HIT_EXACTQ = 0
HIT_IGNOREQ = 0

def alphabeta_quiescence_recurse(board, depth, alpha, beta):

    # return: tuple - score and a list of moves that get to that score

    global NODES, DEBUG, POST, global_chess_position_move_cache
    global HIT_LOWQ, HIT_HIGHQ, HIT_EXACTQ, HIT_IGNOREQ

    NODES += 1
    moves_to_consider = []
    move_list = []

    if board.threefold_repetition():
        return 0, []  # Draw - stop searching this path


    cached_position = global_chess_position_move_cache.probe(board)
    if cached_position is not None:
        move_list, cache_depth, cache_score, cache_node_type, cached_opponent_movelist  = cached_position
        if cache_depth < 0:  # the cache_record was previously inserted from quiescence, so it's ok to use here.
            if cache_node_type == CACHE_SCORE_IGNORE:
                HIT_IGNOREQ += 1
            elif cache_node_type == CACHE_SCORE_EXACT:
                HIT_EXACTQ += 1
                return cache_score, cached_opponent_movelist
            elif cache_node_type == CACHE_SCORE_HIGH and cache_score <= alpha:
                HIT_HIGHQ += 1
                return cache_score, cached_opponent_movelist
            elif cache_node_type == CACHE_SCORE_LOW and cache_score >= beta:
                HIT_LOWQ += 1
                return cache_score, cached_opponent_movelist

    else:
        move_list_generator = chessmove_list.ChessMoveListGenerator(board)
        move_list_generator.generate_move_list(None)
        move_list = move_list_generator.move_list

    if len(move_list) == 0:
        if board.board_attributes & BOARD_IN_CHECK:
            if board.board_attributes & W_TO_MOVE:
                return -100000 + depth, []  # pick sooner vs. later mates
            else:
                return 100000 - depth, []
        else:
            # side cannot move and it is not in check - stalemate
            return 0, []

    # In quiescence, we only consider moves that are captures or promotions
    # Checking all captures makes the game take prohibitively long.  So we need to prune somehow here.  Odd
    # plies are computer to move, Even plies are human to move.

    if depth % 2 == 1:
        for move in move_list:
            if (move[PIECE_CAPTURED] and move[CAPTURE_DIFFERENTIAL] >= 0) or move[PROMOTED_TO]:
                moves_to_consider.append(move)
    else:
        # Only take the move with highest capture differential, which is first in the list, and any promotions.
        if move_list[0][PIECE_CAPTURED] or move_list[0][PROMOTED_TO]:
            moves_to_consider.append(move_list[0])
        for move in move_list[1:]:
            if move[PROMOTED_TO]:
                moves_to_consider.append(move)

    if len(moves_to_consider) == 0:
        return board.evaluate_board(), []
    else:
        mybestmove = None
        best_opponent_bestmovelist = []
        if board.board_attributes & W_TO_MOVE:
            failed_high = False
            local_best_score = -101000
            for move in moves_to_consider:
                board.apply_move(move)
                score, opponent_bestmove_list = alphabeta_quiescence_recurse(board, depth+1, alpha, beta)
                if score > local_best_score:
                    local_best_score = score
                if score > alpha:
                    alpha = score
                    mybestmove = deepcopy(move)
                    best_opponent_bestmovelist = deepcopy(opponent_bestmove_list)
                board.unapply_move()
                if alpha >= beta:
                    failed_high = True
                    break  # alpha-beta cutoff
            if cached_position is None:
                # only put something in the cache if there was nothing previously, as quiescence searches are
                # really only of value for the move list, since it's a limited search.
                if failed_high:
                    cache_score_type = CACHE_SCORE_LOW
                else:
                    cache_score_type = CACHE_SCORE_EXACT
                global_chess_position_move_cache.insert(board, (move_list, -1, local_best_score,
                                                        cache_score_type, [mybestmove] + best_opponent_bestmovelist))
            return alpha, [mybestmove] + best_opponent_bestmovelist
        else:
            failed_low = False
            local_best_score = 101000
            for move in moves_to_consider:
                board.apply_move(move)
                score, opponent_bestmove_list = alphabeta_quiescence_recurse(board, depth+1, alpha, beta)
                if score < local_best_score:
                    local_best_score = score
                if score < beta:
                    beta = score
                    mybestmove = deepcopy(move)
                    best_opponent_bestmovelist = deepcopy(opponent_bestmove_list)
                board.unapply_move()
                if beta <= alpha:
                    break  # alpha-beta cutoff
            if cached_position is None:
                if failed_low:
                    cache_score_type = CACHE_SCORE_HIGH
                else:
                    cache_score_type = CACHE_SCORE_EXACT
                global_chess_position_move_cache.insert(board, (move_list, -1, local_best_score,
                                                        cache_score_type, [mybestmove] + best_opponent_bestmovelist))
            return beta, [mybestmove] + best_opponent_bestmovelist




def alphabeta_recurse(board, current_depth, alpha, beta, target_depth, prev_best_move=None):
    """

    :param board: board being analyzed
    :param current_depth: counted down from original search, so 0 is where we static evaluate)
    :param alpha:
    :param beta:
    :param target_depth: original max depth, needed for debug displays
    :param prev_best_move: for iterative deepening, we can seed the root ply with best move from previous iteration
    :return: tuple - score and a list of moves that get to that score
    """

    # Originally I jumped straight to evaluate_board if depth == 0, but that led to very poor evaluation
    # of positions where the position at exactly depth == 0 was a checkmate.  So no matter what, we check
    # for stalemate / checkmate first, and then we decide whether to recurse or statically evaluate.
    global NODES, DEBUG, POST, global_chess_position_move_cache
    global HIT_HIGH, HIT_LOW, HIT_EXACT, HIT_IGNORE

    NODES += 1
    distance_to_leaves = target_depth - current_depth

    if board.threefold_repetition():
        return 0, []  # Draw - stop searching this path

    cached_position = global_chess_position_move_cache.probe(board)
    if cached_position is not None:
        cached_ml, cache_depth, cache_score, cache_node_type, cached_opponent_movelist  = cached_position
        if cache_depth >= distance_to_leaves:
            if cache_node_type == CACHE_SCORE_IGNORE:
                HIT_IGNORE += 1
            elif cache_node_type == CACHE_SCORE_EXACT:
                HIT_EXACT += 1
                return cache_score, cached_opponent_movelist
            elif cache_node_type == CACHE_SCORE_HIGH and cache_score <= alpha:
                HIT_HIGH += 1
                return cache_score, cached_opponent_movelist
            elif cache_node_type == CACHE_SCORE_LOW and cache_score >= beta:
                HIT_LOW += 1
                return cache_score, cached_opponent_movelist
        if len(cached_ml) == 0:
            move_list = []
        else:
            # To-do - Killer Heuristic goes here
            if prev_best_move is None:
                move_list = cached_ml
            else:
                pos = 0
                for m in cached_ml:
                    if m[START] == prev_best_move[START] and m[END] == prev_best_move[END]:
                        break
                    pos += 1
                move_list = [cached_ml[pos]] + cached_ml[0:pos] + cached_ml[pos+1:]
    else:
        move_list_generator = chessmove_list.ChessMoveListGenerator(board)
        move_list_generator.generate_move_list(last_best_move=prev_best_move)
        move_list = move_list_generator.move_list


    if len(move_list) == 0:
        if board.board_attributes & BOARD_IN_CHECK:
            if board.board_attributes & W_TO_MOVE:
                retval = -100000 + current_depth  # pick sooner vs. later mates
            else:
                retval = 100000 - current_depth
        else:
            # side cannot move and it is not in check - stalemate
            retval = 0
        if cached_position is None:
            global_chess_position_move_cache.insert(board, ([], distance_to_leaves, retval, CACHE_SCORE_EXACT, []))
        return retval, []


    mybestmove = None
    best_opponent_bestmovelist = []
    if board.board_attributes & W_TO_MOVE:
        failed_high = False
        local_best_score = -101000
        for move in move_list:
            board.apply_move(move)
            if current_depth >= target_depth:
                score, opponent_bestmove_list = alphabeta_quiescence_recurse(board, current_depth+1, alpha, beta)
            else:
                score, opponent_bestmove_list = alphabeta_recurse(board, current_depth+1,
                                                              alpha, beta, target_depth, None)
            if score > local_best_score:
                local_best_score = score
            if score > alpha:
                alpha = score
                mybestmove = deepcopy(move)
                best_opponent_bestmovelist = deepcopy(opponent_bestmove_list)
                if current_depth == 1 and POST:
                    print_computer_thoughts(target_depth, alpha, [mybestmove] + best_opponent_bestmovelist)
            board.unapply_move()
            if alpha >= beta:
                failed_high = True
                break  # alpha-beta cutoff - "fail high" - score is stored as lower bound
        if mybestmove is None:
            cache_score_type = CACHE_SCORE_IGNORE
        elif failed_high:
            cache_score_type = CACHE_SCORE_LOW
        else:
            cache_score_type = CACHE_SCORE_EXACT

        global_chess_position_move_cache.insert(board, (move_list, distance_to_leaves, local_best_score,
                                                        cache_score_type, [mybestmove] + best_opponent_bestmovelist))
        return alpha, [mybestmove] + best_opponent_bestmovelist
    else:
        failed_low = False
        local_best_score = 101000
        for move in move_list:
            board.apply_move(move)
            if current_depth >= target_depth:
                score, opponent_bestmove_list = alphabeta_quiescence_recurse(board, current_depth+1, alpha, beta)
            else:
                score, opponent_bestmove_list = alphabeta_recurse(board, current_depth+1,
                                                              alpha, beta, target_depth, None)
            if score < local_best_score:
                local_best_score = score
            if score < beta:
                beta = score
                mybestmove = deepcopy(move)
                best_opponent_bestmovelist = deepcopy(opponent_bestmove_list)
                if current_depth == 1 and POST:
                    print_computer_thoughts(target_depth, beta, [mybestmove] + best_opponent_bestmovelist)
            board.unapply_move()

            if beta <= alpha:
                failed_low = True
                break  # alpha-beta cutoff - "fail low" - score is stored as upper bound
        if mybestmove is None:
            cache_score_type = CACHE_SCORE_IGNORE
        elif failed_low:
            cache_score_type = CACHE_SCORE_HIGH
        else:
            cache_score_type = CACHE_SCORE_EXACT

        global_chess_position_move_cache.insert(board, (move_list, distance_to_leaves, local_best_score,
                                                cache_score_type, [mybestmove] + best_opponent_bestmovelist))
        return beta, [mybestmove] + best_opponent_bestmovelist


def process_computer_move(board, prev_best_move, search_depth=4, search_time=10000):
    global START_TIME, XBOARD

    global HIT_HIGH, HIT_LOW, HIT_EXACT, HIT_HIGHQ, HIT_LOWQ, HIT_EXACTQ, HIT_IGNORE, HIT_IGNOREQ

    HIT_HIGH = 0
    HIT_LOW = 0
    HIT_EXACT = 0
    HIT_HIGHQ = 0
    HIT_LOWQ = 0
    HIT_EXACTQ = 0
    HIT_IGNORE = 0
    HIT_IGNOREQ = 0


    START_TIME = datetime.now()
    if not XBOARD:
        if board.side_to_move_is_in_check():
            print("Check!")

    computer_move_list = chessmove_list.ChessMoveListGenerator(board)
    computer_move_list.generate_move_list()
    half_search_time = search_time // 2  # we will consider additional depth if we have half of our time remaining

    # Iterative deepening.  Start at 2-ply, then increment by 2 plies until we get to the maximum depth.
    # If you start at 1 ply, the move is totally biased towards the capture of the highest value piece possible,
    # and that loses the value of the previous best move.
    best_score, best_move_list = alphabeta_recurse(board, current_depth=1, alpha=-101000, beta=101000,
                                                   target_depth=2, prev_best_move=prev_best_move)

    delta = datetime.now() - START_TIME
    # ms = (1000 * delta.seconds) + (delta.microseconds // 1000)
    ply = 3

    while ply <= search_depth:  # or ms <= half_search_time:
        move = best_move_list[0]
        best_score, best_move_list = alphabeta_recurse(board, current_depth=1,
                                                       alpha=-101000, beta=101000, target_depth=ply,
                                                       prev_best_move=move)
        ply += 1
        delta = datetime.now() - START_TIME
        ms = (1000 * delta.seconds) + (delta.microseconds // 1000)
        if abs(best_score) >= 99900:
            # mate detected, don't bother expanding search
            break


    assert(len(best_move_list) > 0)

    end_time = datetime.now()
    computer_move = best_move_list[0]

    if not XBOARD:
        print("Elapsed time: " + str(end_time - START_TIME))
        print("Move made: %s : Score = %d" % (chessmove_list.pretty_print_move(computer_move, True), best_score))
        movestr = ""
        print ("E%d: H%d: L%d: I%d: EQ:%d HQ:%d LQ:%d IQ:%d" % (HIT_EXACT, HIT_HIGH, HIT_LOW, HIT_IGNORE, HIT_EXACTQ, HIT_HIGHQ, HIT_LOWQ, HIT_IGNOREQ))
        for c in best_move_list:
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
    if len(best_move_list) >= 3:
        return best_move_list[1], best_move_list[2]
    else:
        return None, None


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
    global DEBUG, XBOARD, POST, NODES, DEBUGFILE, global_chess_position_move_cache
    # xboard integration requires us to handle these two signals
    signal.signal(signal.SIGINT, handle_sigint)
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
    NODES = 0

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

    expected_opponent_move = None
    counter_to_expected_opp_move = None

    done_with_current_game = False
    while True:
        print("Cache inserts: %d  Cache hits: %d" % (global_chess_position_move_cache.inserts, global_chess_position_move_cache.probe_hits))


         # only use the expected opponent move / counter if computer vs. human.
        if (computer_is_black and computer_is_white) or (not computer_is_black and not computer_is_white):
            expected_opponent_move = None
            counter_to_expected_opp_move = None

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
            printcommand('feature myname="Bejola0.5"')
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
            if b.board_attributes & W_TO_MOVE:
                computer_is_white = True
                computer_is_black = False
            else:
                computer_is_black = True
                computer_is_white = False
            NODES = 0
            expected_opponent_move, counter_to_expected_opp_move = process_computer_move(b, None, search_depth)
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
                # only add the FEN after the move so we don't slow the compute loop down by computing FEN's
                # that we don't need.  We use the FEN only for draw-by-repetition testing outside the move loop.
                # Inside computer computing moves, we use the hash for speed, giving up some accuracy.
                b.apply_move(human_move)
                b.required_post_move_updates()
                if expected_opponent_move is not None:
                    if (human_move[START] != expected_opponent_move[START] or
                            human_move[END] != expected_opponent_move[END]):
                        counter_to_expected_opp_move = None
                if ((b.board_attributes & W_TO_MOVE) and computer_is_white) or \
                        ((not (b.board_attributes & W_TO_MOVE)) and computer_is_black):
                    done_with_current_game = test_for_end(b)
                    if not done_with_current_game:
                        NODES = 0
                        expected_opponent_move, counter_to_expected_opp_move = \
                            process_computer_move(b, counter_to_expected_opp_move, search_depth)
                        b.required_post_move_updates()

if __name__ == "__main__":
    play_game()
