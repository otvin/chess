cmake_minimum_required(VERSION 3.5)
project(chess)
set (chess_VERSION_MAJOR 0)
set (chess_VERSION_MINOR 1)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")  # enable C99 standard
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(SOURCE_FILES chess.c chess.h generate_moves.c generate_moves.h evaluate_board.c evaluate_board.h chessboard.c chessboard.h chessmove.c chessmove.h check_tables.c check_tables.h chess_constants.h hash.c hash.h random.h)
add_executable(chess ${SOURCE_FILES})

set(UT_SOURCE_FILES chess_unit_tests.c generate_moves.c generate_moves.h evaluate_board.c evaluate_board.h chessboard.c chessboard.h chessmove.c chessmove.h check_tables.c check_tables.h chess_constants.h hash.c hash.h random.h)
add_executable(chess_unit_tests ${UT_SOURCE_FILES})