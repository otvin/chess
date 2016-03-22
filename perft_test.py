# File for doing perft tests to validate the move generation logic.

import cProfile
from datetime import datetime
import chessboard
import chessmove_list
import chesscache


global_movecount = []
global_movecache = chesscache.ChessPositionCache()

def calc_moves(board, depth, is_debug=False):
    global global_movecount, global_movecache

    if depth == 0:
        return

    cached_ml = global_movecache.probe(board)
    if cached_ml is None:
        ml = chessmove_list.ChessMoveListGenerator(board)
        ml.generate_move_list()
        global_movecache.insert(board, ml.move_list)
        local_move_list = ml.move_list
    else:
        local_move_list = cached_ml

    global_movecount[depth-1] += len(local_move_list)


    if is_debug:

        for move in local_move_list:
            try:
                before_fen = board.convert_to_fen()
                board.apply_move(move)
                middle_fen = board.convert_to_fen()
            except:
                print("could not apply move " + move.pretty_print() + " to board " + before_fen)
                print("history:")
                for x in board.move_history:
                    print(x.pretty_print())
                raise

            try:
                calc_moves(board, depth-1)
            except:
                print("previous stack: depth " + str(depth) + ": applied " + move.pretty_print() + " to " + before_fen)
                raise

            try:
                board.unapply_move()
                after_fen = board.convert_to_fen()
            except:
                print("could not unapply move " + move.pretty_print() + " to board " + before_fen + ":" + after_fen)
                raise

            if before_fen != after_fen:
                print(str(depth) + " : " + before_fen + " : " + move.pretty_print() + " : resulted in " + middle_fen + " : then rolled back to: " + after_fen)
                raise AssertionError("Halt")

    else:
        for move in local_move_list:
            board.apply_move(move)
            calc_moves(board, depth-1)
            board.unapply_move()


def perft_test(start_fen, depth):
    for i in range(depth):
        global_movecount.append(0)
    board = chessboard.ChessBoard()
    board.load_from_fen(start_fen)

    start_time = datetime.now()
    try:
        calc_moves(board, depth, is_debug=False)
    except:
        print("Failed to complete " + str(datetime.now() - start_time))
        raise

    end_time = datetime.now()
    for i in range(depth-1, -1, -1):
        print(global_movecount[i])
    print(end_time-start_time, " elapsed time")
    # print (str(global_movecache.inserts) + " cache inserts")
    # print (str(global_movecache.probe_hits) + " cache hits")


# validation from https://chessprogramming.wikispaces.com/Perft+Results

# testing on the start position
# perft_test("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4)
# perft(5) was correct - 4,865,609 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 17 minutes and 47 seconds.
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in calc_moves - 15:17
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 1:54
# 3/21/2016 (v0.3.2+) - caching piece positions, much work on move generation - 1:23
# 3/21/2016 (v0.4+) - chess position cache - 58.9 seconds
# From reading the internet - a decent game can do perft(6) in under 2 minutes, with some under 3 seconds.  So I'm slow.
# Cache results with 1299827 size cache
#   perft(4) - 1,294 hits - 8,029 misses = 13.9% hit rate
#   perft(5) - 94,485 hits - 1,121,119 misses = 7.8% hit rate


# position "3" on that page:
cProfile.run('perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5)')
# perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5)
# perft_test("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5)
# perft(5) was correct - 674,624 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 2 minutes and 45 seconds
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in calc_moves - 2:27
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 0:20
# 3/21/2016 (v0.4+) - chess position cache - 8.18s
# 3/22/2016 (v0.5+) - removing the board member cache, packing bits instead - 7.43/7.48/7.64/8.01 (4 trials)

# perft(6) was correct - 11,030,083 possibilities
# Historical performance: 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 5:52.
# 3/22/2016 (v0.4+) - chess position cache - 1:37

# perft(7) was correct - 178,633,661 possibilities
# 3/22/2016 (v.0.5.1) - chess position cache - 28min 22s.  (Note was doing some minimal activity for part of it)

# cache results with 1299827 size cache:
#   perft(5) - 26,618 hits, 19,638 misses
#   perft(6) - 561,271 hits, 159,609 misses


# position "4" on that page:
# perft_test("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4)
# perft(4) was correct - 422,333 possibilities
# Historical performance: 3/14/2016 (v0.1+) took 1 minute and 37 seconds
# 3/15/2016 (v0.1+) - only test for move to be invalid in certain conditions - 1:24
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in calc_moves - 1:14
# 3/15/2016 (v0.1+) - with apply/unapply instead of copy board in generate move list - 0:11

# perft_test("rn5k/pp6/8/8/8/8/PP6/RN5K w - - 0 1", 5)




"""
0:00:34.833610  elapsed time
         39497159 function calls (38776280 primitive calls) in 34.834 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   34.834   34.834 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:213(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.000    0.000 chessboard.py:283(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:308(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:357(initialize_piece_locations)
        1    0.000    0.000    0.000    0.000 chessboard.py:408(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:476(convert_to_fen)
   370048    0.101    0.000    0.110    0.000 chessboard.py:530(pos_occupied_by_color_moving)
 15893772    3.330    0.000    3.565    0.000 chessboard.py:542(pos_occupied_by_color_not_moving)
        1    0.000    0.000    0.000    0.000 chessboard.py:554(debug_force_recalculation_of_position_score)
  1523869    3.441    0.000    3.973    0.000 chessboard.py:580(unapply_move)
  1523869    0.936    0.000    0.936    0.000 chessboard.py:67(__init__)
  1523869    4.118    0.000    5.519    0.000 chessboard.py:673(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
  1043439    9.037    0.000    9.155    0.000 chessboard.py:816(find_piece)
   997183    6.918    0.000   19.011    0.000 chessboard.py:824(side_to_move_is_in_check)
    46256    0.332    0.000    0.901    0.000 chessboard.py:865(generate_pinned_piece_list)
   802990    0.525    0.000    0.525    0.000 chessmove.py:6(__init__)
    46256    0.286    0.000    0.466    0.000 chessmove_list.py:122(generate_king_moves)
   134059    0.515    0.000    0.734    0.000 chessmove_list.py:160(generate_pawn_moves)
    46256    2.123    0.000   29.500    0.001 chessmove_list.py:207(generate_move_list)
    55623    0.015    0.000    0.015    0.000 chessmove_list.py:333(<lambda>)
    46256    0.026    0.000    0.026    0.000 chessmove_list.py:50(__init__)
   183520    0.503    0.000    0.862    0.000 chessmove_list.py:63(generate_direction_moves)
    45880    0.094    0.000    0.956    0.000 chessmove_list.py:88(generate_slide_moves)
 720880/1    1.028    0.000   34.834   34.834 perft_test.py:12(calc_moves)
        1    0.000    0.000   34.834   34.834 perft_test.py:58(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   34.834   34.834 {built-in method builtins.exec}
    46259    0.009    0.000    0.009    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  6547011    0.538    0.000    0.538    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
  1030718    0.076    0.000    0.076    0.000 {method 'islower' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
  2131016    0.168    0.000    0.168    0.000 {method 'isupper' of 'str' objects}
     3452    0.002    0.000    0.002    0.000 {method 'lower' of 'str' objects}
  1523869    0.154    0.000    0.154    0.000 {method 'pop' of 'list' objects}
  3164481    0.489    0.000    0.489    0.000 {method 'remove' of 'list' objects}
    46256    0.071    0.000    0.086    0.000 {method 'sort' of 'list' objects}
"""

"""
0:00:22.969403  elapsed time
         38453720 function calls (37732841 primitive calls) in 22.970 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   22.970   22.970 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:213(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.000    0.000 chessboard.py:283(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:308(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:357(initialize_piece_locations)
        1    0.000    0.000    0.000    0.000 chessboard.py:408(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:476(convert_to_fen)
   370048    0.100    0.000    0.109    0.000 chessboard.py:530(pos_occupied_by_color_moving)
 15893772    2.889    0.000    3.094    0.000 chessboard.py:542(pos_occupied_by_color_not_moving)
        1    0.000    0.000    0.000    0.000 chessboard.py:554(debug_force_recalculation_of_position_score)
  1523869    3.096    0.000    3.535    0.000 chessboard.py:580(unapply_move)
  1523869    1.000    0.000    1.000    0.000 chessboard.py:67(__init__)
  1523869    3.709    0.000    5.114    0.000 chessboard.py:673(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
  1043439    0.143    0.000    0.143    0.000 chessboard.py:816(find_piece)
   997183    6.235    0.000    9.288    0.000 chessboard.py:825(side_to_move_is_in_check)
    46256    0.258    0.000    0.397    0.000 chessboard.py:866(generate_pinned_piece_list)
   802990    0.481    0.000    0.481    0.000 chessmove.py:6(__init__)
    46256    0.253    0.000    0.417    0.000 chessmove_list.py:122(generate_king_moves)
   134059    0.472    0.000    0.677    0.000 chessmove_list.py:160(generate_pawn_moves)
    46256    1.724    0.000   18.159    0.000 chessmove_list.py:207(generate_move_list)
    55623    0.012    0.000    0.012    0.000 chessmove_list.py:333(<lambda>)
    46256    0.023    0.000    0.023    0.000 chessmove_list.py:50(__init__)
   183520    0.435    0.000    0.768    0.000 chessmove_list.py:63(generate_direction_moves)
    45880    0.071    0.000    0.839    0.000 chessmove_list.py:88(generate_slide_moves)
 720880/1    0.882    0.000   22.969   22.969 perft_test.py:12(calc_moves)
        1    0.000    0.000   22.970   22.970 perft_test.py:58(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   22.970   22.970 {built-in method builtins.exec}
    46259    0.006    0.000    0.006    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  5503572    0.418    0.000    0.418    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
  1030718    0.077    0.000    0.077    0.000 {method 'islower' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
  2131016    0.136    0.000    0.136    0.000 {method 'isupper' of 'str' objects}
     3452    0.001    0.000    0.001    0.000 {method 'lower' of 'str' objects}
  1523869    0.138    0.000    0.138    0.000 {method 'pop' of 'list' objects}
  3164481    0.354    0.000    0.354    0.000 {method 'remove' of 'list' objects}
    46256    0.055    0.000    0.067    0.000 {method 'sort' of 'list' objects}

"""

"""
0:00:17.660300  elapsed time
         19397807 function calls (18676928 primitive calls) in 17.661 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   17.661   17.661 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:213(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.000    0.000 chessboard.py:283(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:308(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:357(initialize_piece_locations)
        1    0.000    0.000    0.000    0.000 chessboard.py:408(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:476(convert_to_fen)
   370048    0.096    0.000    0.108    0.000 chessboard.py:530(pos_occupied_by_color_moving)
   698717    0.177    0.000    0.195    0.000 chessboard.py:542(pos_occupied_by_color_not_moving)
        1    0.000    0.000    0.000    0.000 chessboard.py:554(debug_force_recalculation_of_position_score)
  1523869    2.896    0.000    3.318    0.000 chessboard.py:580(unapply_move)
  1523869    0.834    0.000    0.834    0.000 chessboard.py:67(__init__)
  1523869    3.455    0.000    4.676    0.000 chessboard.py:673(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
    46256    0.011    0.000    0.011    0.000 chessboard.py:816(find_piece)
   997183    4.662    0.000    4.662    0.000 chessboard.py:819(side_to_move_is_in_check)
    46256    0.270    0.000    0.413    0.000 chessboard.py:862(generate_pinned_piece_list)
   802990    0.502    0.000    0.502    0.000 chessmove.py:6(__init__)
    46256    0.254    0.000    0.431    0.000 chessmove_list.py:122(generate_king_moves)
   134059    0.444    0.000    0.664    0.000 chessmove_list.py:160(generate_pawn_moves)
    46256    1.664    0.000   13.070    0.000 chessmove_list.py:207(generate_move_list)
    55623    0.010    0.000    0.010    0.000 chessmove_list.py:333(<lambda>)
    46256    0.020    0.000    0.020    0.000 chessmove_list.py:50(__init__)
   183520    0.444    0.000    0.789    0.000 chessmove_list.py:63(generate_direction_moves)
    45880    0.077    0.000    0.866    0.000 chessmove_list.py:88(generate_slide_moves)
 720880/1    0.880    0.000   17.660   17.660 perft_test.py:12(calc_moves)
        1    0.000    0.000   17.661   17.661 perft_test.py:58(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   17.661   17.661 {built-in method builtins.exec}
    46259    0.005    0.000    0.005    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  5503572    0.389    0.000    0.389    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
   192357    0.018    0.000    0.018    0.000 {method 'islower' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
   105702    0.013    0.000    0.013    0.000 {method 'isupper' of 'str' objects}
     3452    0.001    0.000    0.001    0.000 {method 'lower' of 'str' objects}
  1523869    0.130    0.000    0.130    0.000 {method 'pop' of 'list' objects}
  3164481    0.356    0.000    0.356    0.000 {method 'remove' of 'list' objects}
    46256    0.051    0.000    0.061    0.000 {method 'sort' of 'list' objects}
"""

"""
0:00:17.518978  elapsed time
         18789638 function calls (18068759 primitive calls) in 17.520 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   17.520   17.520 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:213(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.000    0.000 chessboard.py:283(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:308(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:357(initialize_piece_locations)
        1    0.000    0.000    0.000    0.000 chessboard.py:408(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:476(convert_to_fen)
   637132    0.170    0.000    0.189    0.000 chessboard.py:542(pos_occupied_by_color_not_moving)
        1    0.000    0.000    0.000    0.000 chessboard.py:554(debug_force_recalculation_of_position_score)
  1523869    2.903    0.000    3.310    0.000 chessboard.py:580(unapply_move)
  1523869    0.833    0.000    0.833    0.000 chessboard.py:67(__init__)
  1523869    3.546    0.000    4.745    0.000 chessboard.py:673(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
   997183    4.746    0.000    4.746    0.000 chessboard.py:819(side_to_move_is_in_check)
    46256    0.212    0.000    0.213    0.000 chessboard.py:862(generate_pinned_piece_list)
   802990    0.440    0.000    0.440    0.000 chessmove.py:6(__init__)
    46256    0.261    0.000    0.425    0.000 chessmove_list.py:122(generate_king_moves)
   134059    0.448    0.000    0.658    0.000 chessmove_list.py:160(generate_pawn_moves)
    46256    1.647    0.000   12.949    0.000 chessmove_list.py:207(generate_move_list)
    55623    0.011    0.000    0.011    0.000 chessmove_list.py:333(<lambda>)
    46256    0.021    0.000    0.021    0.000 chessmove_list.py:50(__init__)
   183520    0.431    0.000    0.747    0.000 chessmove_list.py:63(generate_direction_moves)
    45880    0.076    0.000    0.824    0.000 chessmove_list.py:88(generate_slide_moves)
 720880/1    0.861    0.000   17.519   17.519 perft_test.py:12(calc_moves)
        1    0.000    0.000   17.519   17.519 perft_test.py:58(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   17.520   17.520 {built-in method builtins.exec}
    46259    0.006    0.000    0.006    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  5503572    0.368    0.000    0.368    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
   156506    0.017    0.000    0.017    0.000 {method 'islower' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
    11273    0.002    0.000    0.002    0.000 {method 'isupper' of 'str' objects}
     3452    0.001    0.000    0.001    0.000 {method 'lower' of 'str' objects}
  1523869    0.135    0.000    0.135    0.000 {method 'pop' of 'list' objects}
  3164481    0.329    0.000    0.329    0.000 {method 'remove' of 'list' objects}
    46256    0.052    0.000    0.063    0.000 {method 'sort' of 'list' objects}

"""

"""
0:00:16.395077  elapsed time
         17984727 function calls (17263848 primitive calls) in 16.396 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   16.396   16.396 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:217(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.000    0.000 chessboard.py:287(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:312(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:361(initialize_piece_locations)
        1    0.000    0.000    0.000    0.000 chessboard.py:412(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:480(convert_to_fen)
        1    0.000    0.000    0.000    0.000 chessboard.py:554(debug_force_recalculation_of_position_score)
  1523869    2.753    0.000    3.130    0.000 chessboard.py:580(unapply_move)
  1523869    0.789    0.000    0.789    0.000 chessboard.py:67(__init__)
  1523869    3.338    0.000    4.471    0.000 chessboard.py:673(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
   997183    4.495    0.000    4.495    0.000 chessboard.py:819(side_to_move_is_in_check)
    46256    0.206    0.000    0.208    0.000 chessboard.py:862(generate_pinned_piece_list)
   802990    0.419    0.000    0.419    0.000 chessmove.py:6(__init__)
    46256    0.238    0.000    0.342    0.000 chessmove_list.py:132(generate_king_moves)
   134059    0.414    0.000    0.551    0.000 chessmove_list.py:176(generate_pawn_moves)
    46256    1.578    0.000   12.058    0.000 chessmove_list.py:226(generate_move_list)
    55623    0.011    0.000    0.011    0.000 chessmove_list.py:352(<lambda>)
    46256    0.020    0.000    0.020    0.000 chessmove_list.py:50(__init__)
   183520    0.420    0.000    0.656    0.000 chessmove_list.py:63(generate_direction_moves)
    45880    0.074    0.000    0.729    0.000 chessmove_list.py:92(generate_slide_moves)
 720880/1    0.809    0.000   16.395   16.395 perft_test.py:12(calc_moves)
        1    0.000    0.000   16.396   16.396 perft_test.py:58(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   16.396   16.396 {built-in method builtins.exec}
    46259    0.005    0.000    0.005    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  5503572    0.352    0.000    0.352    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
     3452    0.001    0.000    0.001    0.000 {method 'lower' of 'str' objects}
  1523869    0.118    0.000    0.118    0.000 {method 'pop' of 'list' objects}
  3164481    0.308    0.000    0.308    0.000 {method 'remove' of 'list' objects}
    46256    0.046    0.000    0.057    0.000 {method 'sort' of 'list' objects}
"""

"""
0:00:16.191427  elapsed time
         17981286 function calls (17260407 primitive calls) in 16.192 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   16.192   16.192 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:217(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.000    0.000 chessboard.py:287(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:312(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:361(initialize_piece_locations)
        1    0.000    0.000    0.000    0.000 chessboard.py:412(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:480(convert_to_fen)
        1    0.000    0.000    0.000    0.000 chessboard.py:554(debug_force_recalculation_of_position_score)
  1523869    2.752    0.000    3.139    0.000 chessboard.py:580(unapply_move)
  1523869    0.760    0.000    0.760    0.000 chessboard.py:67(__init__)
  1523869    3.260    0.000    4.361    0.000 chessboard.py:673(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
   997183    4.494    0.000    4.494    0.000 chessboard.py:817(side_to_move_is_in_check)
    46256    0.203    0.000    0.204    0.000 chessboard.py:860(generate_pinned_piece_list)
   802990    0.437    0.000    0.437    0.000 chessmove.py:6(__init__)
    46256    0.233    0.000    0.340    0.000 chessmove_list.py:132(generate_king_moves)
   134059    0.398    0.000    0.539    0.000 chessmove_list.py:176(generate_pawn_moves)
    46256    1.533    0.000   11.965    0.000 chessmove_list.py:226(generate_move_list)
    55623    0.010    0.000    0.010    0.000 chessmove_list.py:352(<lambda>)
    46256    0.018    0.000    0.018    0.000 chessmove_list.py:50(__init__)
   183520    0.411    0.000    0.656    0.000 chessmove_list.py:63(generate_direction_moves)
    45880    0.070    0.000    0.726    0.000 chessmove_list.py:92(generate_slide_moves)
 720880/1    0.777    0.000   16.191   16.191 perft_test.py:12(calc_moves)
        1    0.000    0.000   16.192   16.192 perft_test.py:58(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   16.192   16.192 {built-in method builtins.exec}
    46259    0.004    0.000    0.004    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  5503572    0.352    0.000    0.352    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
       11    0.000    0.000    0.000    0.000 {method 'lower' of 'str' objects}
  1523869    0.122    0.000    0.122    0.000 {method 'pop' of 'list' objects}
  3164481    0.311    0.000    0.311    0.000 {method 'remove' of 'list' objects}
    46256    0.045    0.000    0.055    0.000 {method 'sort' of 'list' objects}

"""

"""
ADDED CACHING

0:00:10.191194  elapsed time
         12138299 function calls (11417420 primitive calls) in 10.197 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000   10.197   10.197 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:216(initialize_psts)
        2    0.000    0.000    0.000    0.000 chessboard.py:27(arraypos_to_algebraic)
        1    0.000    0.000    0.005    0.005 chessboard.py:286(__init__)
        2    0.000    0.000    0.005    0.003 chessboard.py:311(erase_board)
        3    0.005    0.002    0.005    0.002 chessboard.py:360(initialize_piece_locations)
    46502    0.077    0.000    0.120    0.000 chessboard.py:369(quickstring)
        1    0.000    0.000    0.000    0.000 chessboard.py:417(load_from_fen)
       42    0.001    0.000    0.001    0.000 chessboard.py:485(convert_to_fen)
        1    0.000    0.000    0.000    0.000 chessboard.py:559(debug_force_recalculation_of_position_score)
  1064971    1.924    0.000    2.183    0.000 chessboard.py:585(unapply_move)
  1064971    0.542    0.000    0.542    0.000 chessboard.py:67(__init__)
  1064971    2.324    0.000    3.104    0.000 chessboard.py:678(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
   429513    1.896    0.000    1.896    0.000 chessboard.py:822(side_to_move_is_in_check)
    19636    0.087    0.000    0.088    0.000 chessboard.py:865(generate_pinned_piece_list)
    65892    0.220    0.000    0.220    0.000 chesscache.py:45(compute_hash)
    19636    0.019    0.000    0.153    0.000 chesscache.py:70(insert)
    46256    0.048    0.000    0.254    0.000 chesscache.py:75(probe)
   344092    0.380    0.000    0.380    0.000 chessmove.py:6(__init__)
    19636    0.110    0.000    0.201    0.000 chessmove_list.py:132(generate_king_moves)
    56571    0.184    0.000    0.260    0.000 chessmove_list.py:176(generate_pawn_moves)
    19636    0.687    0.000    5.515    0.000 chessmove_list.py:226(generate_move_list)
    23693    0.004    0.000    0.004    0.000 chessmove_list.py:331(<lambda>)
    19636    0.008    0.000    0.008    0.000 chessmove_list.py:50(__init__)
    77568    0.304    0.000    0.547    0.000 chessmove_list.py:63(generate_direction_moves)
    19392    0.038    0.000    0.585    0.000 chessmove_list.py:92(generate_slide_moves)
 720880/1    0.742    0.000   10.191   10.191 perft_test.py:13(calc_moves)
        1    0.000    0.000   10.197   10.197 perft_test.py:69(perft_test)
        2    0.000    0.000    0.000    0.000 {built-in method builtins.chr}
        1    0.000    0.000   10.197   10.197 {built-in method builtins.exec}
    46259    0.004    0.000    0.004    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  3625325    0.232    0.000    0.232    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
    46502    0.043    0.000    0.043    0.000 {method 'join' of 'str' objects}
       11    0.000    0.000    0.000    0.000 {method 'lower' of 'str' objects}
  1064971    0.081    0.000    0.081    0.000 {method 'pop' of 'list' objects}
  2212071    0.214    0.000    0.214    0.000 {method 'remove' of 'list' objects}
    19636    0.021    0.000    0.026    0.000 {method 'sort' of 'list' objects}

"""

"""
SHUT OFF DEBUG IN THIS MODULE

0:00:09.871782  elapsed time
         12134709 function calls (11413830 primitive calls) in 9.878 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000    9.878    9.878 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:216(initialize_psts)
        1    0.000    0.000    0.006    0.006 chessboard.py:286(__init__)
        2    0.000    0.000    0.006    0.003 chessboard.py:311(erase_board)
        3    0.006    0.002    0.006    0.002 chessboard.py:360(initialize_piece_locations)
    46476    0.074    0.000    0.116    0.000 chessboard.py:369(quickstring)
        1    0.000    0.000    0.000    0.000 chessboard.py:417(load_from_fen)
        1    0.000    0.000    0.000    0.000 chessboard.py:559(debug_force_recalculation_of_position_score)
  1064708    1.814    0.000    2.071    0.000 chessboard.py:585(unapply_move)
  1064708    0.527    0.000    0.527    0.000 chessboard.py:67(__init__)
  1064708    2.229    0.000    2.988    0.000 chessboard.py:678(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
   429198    1.897    0.000    1.897    0.000 chessboard.py:822(side_to_move_is_in_check)
    19623    0.086    0.000    0.086    0.000 chessboard.py:865(generate_pinned_piece_list)
    65879    0.219    0.000    0.219    0.000 chesscache.py:45(compute_hash)
    19623    0.021    0.000    0.154    0.000 chesscache.py:70(insert)
    46256    0.051    0.000    0.253    0.000 chesscache.py:75(probe)
   343829    0.292    0.000    0.292    0.000 chessmove.py:6(__init__)
    19623    0.133    0.000    0.197    0.000 chessmove_list.py:132(generate_king_moves)
    56534    0.177    0.000    0.248    0.000 chessmove_list.py:176(generate_pawn_moves)
    19623    0.660    0.000    5.360    0.000 chessmove_list.py:226(generate_move_list)
    23670    0.004    0.000    0.004    0.000 chessmove_list.py:331(<lambda>)
    19623    0.008    0.000    0.008    0.000 chessmove_list.py:50(__init__)
    77516    0.346    0.000    0.531    0.000 chessmove_list.py:63(generate_direction_moves)
    19379    0.037    0.000    0.568    0.000 chessmove_list.py:92(generate_slide_moves)
 720880/1    0.712    0.000    9.872    9.872 perft_test.py:13(calc_moves)
        1    0.000    0.000    9.878    9.878 perft_test.py:69(perft_test)
        1    0.000    0.000    9.878    9.878 {built-in method builtins.exec}
    46259    0.004    0.000    0.004    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  3624232    0.224    0.000    0.224    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
    46476    0.042    0.000    0.042    0.000 {method 'join' of 'str' objects}
       11    0.000    0.000    0.000    0.000 {method 'lower' of 'str' objects}
  1064708    0.082    0.000    0.082    0.000 {method 'pop' of 'list' objects}
  2211518    0.210    0.000    0.210    0.000 {method 'remove' of 'list' objects}
    19623    0.020    0.000    0.025    0.000 {method 'sort' of 'list' objects}

"""
"""
REMOVE CHESS BOARD MEMBER CACHE, REPLACE WITH BITPACKING

0:00:09.555198  elapsed time
         11198612 function calls (10477733 primitive calls) in 9.556 seconds

   Ordered by: standard name

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000    9.556    9.556 <string>:1(<module>)
        1    0.000    0.000    0.000    0.000 chessboard.py:205(initialize_psts)
        1    0.000    0.000    0.000    0.000 chessboard.py:282(__init__)
        2    0.000    0.000    0.000    0.000 chessboard.py:307(erase_board)
        3    0.000    0.000    0.000    0.000 chessboard.py:353(initialize_piece_locations)
    47506    0.052    0.000    0.099    0.000 chessboard.py:362(quickstring)
        1    0.000    0.000    0.000    0.000 chessboard.py:407(load_from_fen)
        1    0.000    0.000    0.000    0.000 chessboard.py:544(debug_force_recalculation_of_position_score)
  1071704    1.638    0.000    1.901    0.000 chessboard.py:570(unapply_move)
  1071704    2.339    0.000    2.572    0.000 chessboard.py:659(apply_move)
        1    0.000    0.000    0.000    0.000 chessboard.py:7(algebraic_to_arraypos)
   437919    1.994    0.000    1.994    0.000 chessboard.py:794(side_to_move_is_in_check)
    20034    0.089    0.000    0.090    0.000 chessboard.py:837(generate_pinned_piece_list)
    86324    0.311    0.000    0.311    0.000 chesscache.py:56(compute_hash)
    20034    0.027    0.000    0.161    0.000 chesscache.py:81(insert)
    66290    0.064    0.000    0.341    0.000 chesscache.py:87(probe)
   350825    0.319    0.000    0.319    0.000 chessmove.py:6(__init__)
    19785    0.040    0.000    0.592    0.000 chessmove_list.py:104(generate_slide_moves)
    20034    0.143    0.000    0.226    0.000 chessmove_list.py:144(generate_king_moves)
    57713    0.233    0.000    0.321    0.000 chessmove_list.py:188(generate_pawn_moves)
    20034    0.678    0.000    5.522    0.000 chessmove_list.py:238(generate_move_list)
    24175    0.005    0.000    0.005    0.000 chessmove_list.py:354(<lambda>)
    20034    0.009    0.000    0.009    0.000 chessmove_list.py:62(__init__)
    79140    0.376    0.000    0.552    0.000 chessmove_list.py:75(generate_direction_moves)
 720880/1    0.640    0.000    9.555    9.555 perft_test.py:13(calc_moves)
        1    0.000    0.000    9.556    9.556 perft_test.py:69(perft_test)
        1    0.000    0.000    9.556    9.556 {built-in method builtins.exec}
    46259    0.004    0.000    0.004    0.000 {built-in method builtins.len}
        1    0.000    0.000    0.000    0.000 {built-in method builtins.ord}
        6    0.000    0.000    0.000    0.000 {built-in method builtins.print}
        2    0.000    0.000    0.000    0.000 {built-in method now}
  3652866    0.230    0.000    0.230    0.000 {method 'append' of 'list' objects}
        1    0.000    0.000    0.000    0.000 {method 'disable' of '_lsprof.Profiler' objects}
        2    0.000    0.000    0.000    0.000 {method 'find' of 'str' objects}
        1    0.000    0.000    0.000    0.000 {method 'isnumeric' of 'str' objects}
    47506    0.047    0.000    0.047    0.000 {method 'join' of 'str' objects}
       11    0.000    0.000    0.000    0.000 {method 'lower' of 'str' objects}
  1071704    0.082    0.000    0.082    0.000 {method 'pop' of 'list' objects}
  2226071    0.212    0.000    0.212    0.000 {method 'remove' of 'list' objects}
    20034    0.022    0.000    0.027    0.000 {method 'sort' of 'list' objects}
"""
