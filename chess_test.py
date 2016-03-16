import chess

# This is my file for testing the chess.py engine in various positions, as well as tracking its historical performance
# so I can compare logic improvements and performance improvements.



# Mate in one tests - http://open-chess.org/viewtopic.php?f=7&t=997
# play_game("2N5/4R3/2k3KQ/R7/1PB5/5N2/8/6B1 w - - 0 1", True, True, False)
# play_game("4N3/5P1P/5N1k/Q5p1/5PKP/B7/8/1B6 w - - 0 1", True, True, False)
# play_game("8/4N3/7Q/4k3/8/4KP2/3P4/8 w - - 0 1", True, True, False)
# play_game("r3kb2/5p2/8/n2K1p2/1pP5/3P1n2/b7/8 b q c3 0 1", True, False, True)

# Mate in two
chess.play_game("B7/K1B1p1Q1/5r2/7p/1P1kp1bR/3P3R/1P1NP3/2n5 w - - 0 1", computer_is_white=True, is_debug=True)

# chess.play_game(is_debug=True, computer_is_black=True)


# Historical performance
# 3/15/2016 - apply/unapply replaced copy in generate move list - search depth 3
#   e2-e4 . e7-e5 (0 points, 8.92s)
#   g1-f3 . d7-d5 (20 points, 15.37s)
#   b1-c3 . d5xe4 (95 points, 29.15s)
#   c3xe4 . g8-e7 (295 points, 34.24s)
#   f1-c4 . b8-d7 (330 points, 33.01s)
#   e1-g1 . g7-g5 (305 points, 16.47s)
#   f3xg5 . h8-g8 (210 points, 21.17s)
#   d1-h5 . g8-g7 (650 points, 24.02s) -- I think it may be thinking it will win N & Q for R, negamax may be wrong
#   g5xh7 . g7xh7 (870 points, 24.55s) -- definitely
#   h5xh7 . f7-f5 (470 points, 18.01s) -- missed the mate
#   h7-f7+.  1-0

# 3/16/2016 - fixed the mini/max so now it "thinks."  search depth 3
#   e2-e4 . g8-f6 (-55 points, 12.49s)
#   b1-c3 . b8-c6 (-90 points, 14.77s)
#   g1-f3 . e7-e5 (-90 points, 15.28s)
#   f1-c4 . f8-b4 (-130 points, 27.96s)
#   a2-a3 . b4-a5 (-115 points, 36.61s)
#   b2-b4 . a5-b6 (-100 points, 27.37s)
#   d2-d3 .  O-O  (-120 points, 36.96s)
#   c1-g5 . d7-d5 (-55 points, 34.21s)
#   c4xd5 . b6xf2+(-45 points, 49.07s)
#   e1xf2 . f6-g4+(-10 points, 52.15s)
#   f2-g1 . c6-e7 (315 points, 35.42s)
#   f3xe5 . g4xe5 (90 points, 40.54s)
#   d3-d4 . c8-g4 (-105 points, 40.73s)
#   d1-d2 . e5-f3+(95 points, 53.05s)
#   g2xf3 . g4xf3 (165 points, 39.73s)
#   d5xb7 . f3xh1 (155 points, 44.76s)
#   b7xa8 . d8xa8 (325 points, 28.62s)
#   g1xh1 . e7-f5 (325 points, 22.22s)
#   a1-e1 . a8-c6 (310 points, 24.76s)
#   d4-d5 . c6-e8 (335 points, 39.51s)
#   e4xf5 . e8-a8 (655 points, 21.38s)
#   f5-f6 . g7xf6 (645 points, 14.77s)
#   g5xf6 . h7-h6 (805 points, 13.13s)
#   d2-g2+. g8-h7 (32000 points, 0.80s)
#   g2-g7+.  1-0