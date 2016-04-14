cmake_minimum_required(VERSION 3.5)
project(chess)
set (chess_VERSION_MAJOR 0)
set (chess_VERSION_MINOR 1)


set(SOURCE_FILES chess.c chess.h chessboard.c chessboard.h chessmove.c chessmove.h chess_constants.h)
add_executable(chess ${SOURCE_FILES})