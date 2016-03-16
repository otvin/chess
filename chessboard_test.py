import chessboard
import chessmove_list
from chessmove import ChessMove


def test_algebraic_to_arraypos_converters():
    num_failures = 0
    num_successes = 0

    for file in range(97, 105, 1):
        for rank in range(1, 9, 1):
            algebraicpos = chr(file) + str(rank)
            converted_algebraicpos = chessboard.arraypos_to_algebraic(chessboard.algebraic_to_arraypos(algebraicpos))
            if converted_algebraicpos != algebraicpos:
                print("Failure: ", algebraicpos, " converted to ", converted_algebraicpos)
                num_failures += 1
            else:
                num_successes += 1
    print("Complete. Success: ", num_successes, " Failure: ", num_failures)


def test_fen_load():
    b = chessboard.ChessBoard()
    b.erase_board()
    # b.load_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    # b.load_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1")
    b.load_from_fen("rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    print(b.pretty_print())
    print(b.white_can_castle_king_side)
    print(b.white_can_castle_king_side)
    print(b.black_can_castle_king_side)
    print(b.black_can_castle_queen_side)
    print(b.halfmove_clock)
    print(b.fullmove_number)
    print(b.en_passant_target_square)


def test_fen_save():
    b = chessboard.ChessBoard()
    b.initialize_start_position()
    print(b.convert_to_fen())


def test_queen_moves():
    b = chessboard.ChessBoard()
    b.load_from_fen("k7/8/8/8/3Qp3/8/8/7K w - - 0 99")
    l = chessmove_list.ChessMoveList(b)
    l.generate_move_list()
    print(l.pretty_print())


def test_all_moves(pos):
    b = chessboard.ChessBoard()
    b.load_from_fen(pos)
    l = chessmove_list.ChessMoveList(b)
    l.generate_move_list()
    print(l.pretty_print())



def test_a_check(fen, position_name, supposed_to_be_check):
    b = chessboard.ChessBoard()
    b.load_from_fen(fen)
    if b.side_to_move_is_in_check() == supposed_to_be_check:
        print(position_name, " succeeded")
    else:
        print(position_name, " failed")
        print(b.pretty_print())
        raise RuntimeError("Halt")


def test_checks():
    test_a_check("K7/8/8/Q7/8/7k/8/8 w - - 1 1", "Q1", False)
    test_a_check("k7/8/8/Q7/8/7K/8/8 w - - 1 1", "Q2", False)
    test_a_check("k7/8/8/Q7/8/7K/8/8 b - - 1 1", "Q3", True)
    test_a_check("k7/p7/8/Q7/8/7K/8/8 b - - 1 1", "Q4", False)
    test_a_check("k7/p7/8/3Q4/8/7K/8/8 b - - 1 1", "Q5", True)

    test_a_check("k7/p7/8/3B4/8/7K/8/8 b - - 1 1", "B1", True)
    test_a_check("k7/p7/8/3b4/8/7K/8/8 b - - 1 1", "B2", False)
    test_a_check("k7/pp6/8/3B4/8/7K/8/8 b - - 1 1", "B3", False)

    test_a_check("K7/8/8/R7/8/7k/8/8 w - - 1 1", "R1", False)
    test_a_check("k7/8/8/R7/8/7K/8/8 w - - 1 1", "R2", False)
    test_a_check("k7/8/8/R7/8/7K/8/8 b - - 1 1", "R3", True)

    test_a_check("k7/pP6/8/3b4/8/7K/8/8 b - - 1 1", "P1", True)
    test_a_check("1k6/Pp6/8/3b4/8/7K/8/8 b - - 1 1", "P2", True)
    test_a_check("4N3/5P1P/5N1k/Q5p1/5PKP/B7/8/1B6 w - - 0 1", "P3", False)
    test_a_check("4N3/5P1P/5N1k/Q5P1/6KP/B7/8/1B6 b - - 0 1", "P4", True)

    test_a_check("k7/2N5/8/8/8/8/8/K7 b - - 1 1", "N1", True)

    test_a_check("kK6/8/8/8/8/8/8/8 b - - 1 1 ", "K1", True)
    test_a_check("kK6/8/8/8/8/8/8/8 w - - 1 1 ", "K2", True)

def test_a_moveapply(start_fen, move, end_fen, position_name, pretty_print = False):
    b = chessboard.ChessBoard()
    b.load_from_fen(start_fen)

    cache = chessboard.ChessBoardMemberCache(b)

    b.apply_move(move)
    res = b.convert_to_fen()
    if res == end_fen:
        print(position_name, " succeeded")
        if pretty_print:
            print(b.pretty_print())
    else:
        print(position_name, " failed")
        print(res)
        print(b.pretty_print())
        raise RuntimeError("Halt")

    b.unapply_move(move, cache)
    res = b.convert_to_fen()
    if res == start_fen:
        print(position_name, " rollback succeeded")
    else:
        print(position_name, " rollback failed")
        print(start_fen)
        print(res)
        raise RuntimeError("Halt")


def test_apply_move():
    test_a_moveapply("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",ChessMove(35,55,is_two_square_pawn_move=True),
                     "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", "P1")
    test_a_moveapply("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", ChessMove(84,64,is_two_square_pawn_move=True),
                     "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", "P2")
    test_a_moveapply("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", ChessMove(22,43),
                     "rnbqkbnr/ppp1pppp/8/3p4/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 1 2", "P3")
    test_a_moveapply("rnbqkbnr/ppp1pppp/8/3p4/4P3/2N5/PPPP1PPP/R1BQKBNR b KQkq - 1 2", ChessMove(64,54),
                     "rnbqkbnr/ppp1pppp/8/8/3pP3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 0 3", "P4")
    test_a_moveapply("rnbqkbnr/ppp1pppp/8/8/3pP3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 0 3", ChessMove(24,46),
                     "rnbqkbnr/ppp1pppp/8/8/3pP3/2N2Q2/PPPP1PPP/R1B1KBNR b KQkq - 1 3", "P5")
    test_a_moveapply("rnbqkbnr/ppp1pppp/8/8/3pP3/2N2Q2/PPPP1PPP/R1B1KBNR b KQkq - 1 3", ChessMove(54,43,is_capture=True, piece_captured="N"),
                     "rnbqkbnr/ppp1pppp/8/8/4P3/2p2Q2/PPPP1PPP/R1B1KBNR w KQkq - 0 4", "P6")
    test_a_moveapply("rnbqkbnr/pPpppppp/8/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 5", ChessMove(82,91,is_capture=True, piece_captured="r", is_promotion = True,
                                                                                           promoted_to="Q"),
                        "Qnbqkbnr/p1pppppp/8/8/8/8/1PPPPPPP/RNBQKBNR b KQkq - 0 5", "P7")
    test_a_moveapply("rnbqkbnr/pppppppp/P7/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 5", ChessMove(71, 82, is_capture=True, piece_captured="p"),
                        "rnbqkbnr/pPpppppp/8/8/8/8/1PPPPPPP/RNBQKBNR b KQkq - 0 5", "P8")




    test_a_moveapply("rnbqkbnr/ppp1pppp/8/8/4P3/2p2Q2/PPPP1PPP/R3KBNR w KQkq - 0 4", ChessMove(21,22),
                     "rnbqkbnr/ppp1pppp/8/8/4P3/2p2Q2/PPPP1PPP/1R2KBNR b Kkq - 1 4", "Castle 1")
    test_a_moveapply("rnbqkbnr/ppp1pppp/8/8/4P3/2p2Q2/PPPP1PPP/R3KBNR w KQkq - 0 4", ChessMove(25,23,is_castle=True),
                     "rnbqkbnr/ppp1pppp/8/8/4P3/2p2Q2/PPPP1PPP/2KR1BNR b kq - 1 4", "Castle 2")

    test_a_moveapply("rn5k/8/p7/Pp6/8/8/1P6/RN5K w - b6 0 3", ChessMove(61,72,is_capture=True, piece_captured="p", is_en_passant_capture=True),
                     "rn5k/8/pP6/8/8/8/1P6/RN5K b - - 0 3", "EP1")


def test_a_pinned_piece_position(start_fen, pretty_print = False):
    b = chessboard.ChessBoard()
    b.load_from_fen(start_fen)
    pinlist = b.generate_pinned_piece_list()
    if pretty_print:
        print(b.pretty_print(True))
    if len(pinlist) == 0:
        print ("No pinned pieces")
    else:
        for square in pinlist:
            print ("piece at " + chessboard.arraypos_to_algebraic(square) + " is pinned.")



def test_pinned_piece_list():
    test_a_pinned_piece_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", True)
    test_a_pinned_piece_position("k7/p7/8/8/8/Q7/K7/8 w - - 0 1", True)
    test_a_pinned_piece_position("k7/p7/8/8/8/Q7/K7/8 b - - 0 1", True)
    test_a_pinned_piece_position("k7/p7/8/8/8/R7/K7/8 b - - 0 1", True)
    test_a_pinned_piece_position("k7/n7/8/8/8/R7/K7/8 b - - 0 1", True)
    test_a_pinned_piece_position("k7/n7/8/8/8/B7/K7/8 b - - 0 1", True)
    test_a_pinned_piece_position("k7/nb6/8/8/8/B7/K7/7Q b - - 0 1", True)
    test_a_pinned_piece_position("k7/nb6/8/8/8/R7/K7/7Q b - - 0 1", True)
    test_a_pinned_piece_position("k7/nb6/r7/8/8/R7/K7/7Q w - - 0 1", True)
    test_a_pinned_piece_position("8/8/8/KP5r/1R3p1k/8/6P1/8 w - - 0 1", True)
    test_a_pinned_piece_position("8/8/8/KP5r/1R3p1k/8/6P1/8 b - - 0 1", True)

def test_movelist_generation():
    #b = chessboard.ChessBoard()
    #b.initialize_start_position()
    #ml = chessmove_list.ChessMoveList(b)
    #ml.generate_move_list()
    #print(ml.pretty_print())

    #print ("------")

    b = chessboard.ChessBoard()
    #b.load_from_fen("k7/8/pP6/8/8/8/Q7/K7 w - a7 1 1")
    #b.load_from_fen("rnbqkbnr/1p1p2pp/2p1p3/5Q2/p1B1P3/5N2/PPPP1PPP/RNB1K2R w KQkq - 1 6")
    # b.load_from_fen("8/8/8/KP5r/1R3p1k/8/6P1/8 w - - 0 1")
    # b.load_from_fen("8/8/8/KP5r/1R3pPk/8/8/8 b - g3 0 1")
    # b.load_from_fen("4N3/5P1P/5N1k/Q5p1/5PKP/B7/8/1B6 w - - 0 1")
    b.load_from_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/3q1N2/Pp1P1RPP/R2Q2K1 w kq - 2 2")

    print (b.pretty_print(False))
    ml = chessmove_list.ChessMoveListGenerator(b)
    ml.generate_move_list()
    print(ml.pretty_print())



def test_human_input():
    b = chessboard.ChessBoard()
    b.initialize_start_position()
    m = chessmove_list.return_validated_move(b,"e2-e4")
    if m is None:
        print("invalid move")
    else:
        m.pretty_print()
        b.apply_move(m)
        print(b.pretty_print(True))

# test_pinned_piece_list()
test_movelist_generation()
# test_checks()
# test_apply_move()
