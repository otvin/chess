import chessboard

def test_algebraic_to_arraypos_converters():
    num_failures = 0
    num_successes = 0

    for file in range(97, 105, 1):
        for rank in range (1, 9, 1):
            algebraicpos = chr(file) + str(rank)
            converted_algebraicpos = chessboard.arraypos_to_algebraic(chessboard.algebraic_to_arraypos(algebraicpos))
            if converted_algebraicpos != algebraicpos:
                print("Failure: ", algebraicpos, " converted to ", converted_algebraicpos)
                num_failures += 1
            else:
                num_successes += 1
    print ("Complete. Success: ", num_successes, " Failure: ", num_failures)



def test_fen_load():
    b = chessboard.ChessBoard()
    b.erase_board()
    #b.load_from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    #b.load_from_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1")
    b.load_from_fen("rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2")
    print(b.pretty_print())
    print(b.white_castle_king_side)
    print(b.white_castle_queen_side)
    print(b.black_castle_king_side)
    print(b.black_castle_queen_side)
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
    b.generate_move_list()
    print(b.pretty_print_movelist())


def test_all_moves(pos):
    b = chessboard.ChessBoard()
    b.load_from_fen(pos)
    b.generate_move_list()
    print(b.pretty_print_movelist())


# test_queen_moves()