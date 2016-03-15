import chess


# play_game(True, "k7/8/pP6/8/8/8/Q7/K7 w - a7 145 102")
# play_game(debug_fen = "1K6/7q/1k6/8/8/8/8/8 b - - 50 50", is_debug = True)
# play_game(debug_fen ="2bqkbn1/2pppp2/np2N3/r3P1p1/p2N2B1/5Q2/PPPPKPP1/RNB2r2 w KQkq - 0 1", is_debug = True)



# Mate in one tests - http://open-chess.org/viewtopic.php?f=7&t=997
# play_game("2N5/4R3/2k3KQ/R7/1PB5/5N2/8/6B1 w - - 0 1", True, True, False)
# play_game("4N3/5P1P/5N1k/Q5p1/5PKP/B7/8/1B6 w - - 0 1", True, True, False)
# play_game("8/4N3/7Q/4k3/8/4KP2/3P4/8 w - - 0 1", True, True, False)
# play_game("r3kb2/5p2/8/n2K1p2/1pP5/3P1n2/b7/8 b q c3 0 1", True, False, True)

chess.play_game("r1bqkb1r/pppppppp/2n5/8/2B5/3P1Q2/PPP2KPP/RNB3NR b kq - 0 1", True, False, True)