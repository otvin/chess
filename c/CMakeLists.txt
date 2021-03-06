cmake_minimum_required(VERSION 3.5)
project(chess)
set (chess_VERSION_MAJOR 0)
set (chess_VERSION_MINOR 1)


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=haswell -std=gnu11")
# set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -fno-inline")  # enable profiling

# More speedups
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fomit-frame-pointer")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funsafe-loop-optimizations")


set(CMAKE_VERBOSE_MAKEFILE ON)

set (COMMON_SOURCE_FILES generate_moves.c generate_moves.h evaluate_board.c evaluate_board.h chessboard.c chessboard.h chessmove.c chessmove.h check_tables.c)
set (COMMON_SOURCE_FILES ${COMMON_SOURCE_FILES} check_tables.h chess_constants.h hash.c hash.h random.h bitboard.h bitboard.c magicmoves.h magicmoves.c)

set(SOURCE_FILES ${COMMON_SOURCE_FILES} chess.c chess.h)
add_executable(chess ${SOURCE_FILES})

set(UT_SOURCE_FILES ${COMMON_SOURCE_FILES} chess_unit_tests.c)
add_executable(chess_unit_tests ${UT_SOURCE_FILES})