#include <stdlib.h>
#include <stdio.h>

#include "chessmove.h"
#include "generate_moves.h"
#include "evaluate_board.h"

void print_move_list(const struct MoveList *list)
{
    char *movestr;
    int i;

    for (i = 0; i < list->size; i++) {
        movestr = pretty_print_move(list->moves[i]);
        printf("%s\n", movestr);
        free(movestr);
    }
}

void movelist_remove(MoveList *ml, int position)
{
    // concept taken from GNUChess 6
    // removes the move at the specified position in the list
    int i;

    if (position >= ml->size) {
        return;  // TODO - convert to assertion of some form.
    }

    for (i = position; i < ml->size-1; i++) {
        ml->moves[i] = ml->moves[i+1];
    }

    ml->size--;
}

void squarelist_remove(SquareList *sl, int position)
{
    int i;
    if (position >= sl->size) {
        return;  // TODO - convert to assert
    }
    for (i = position; i < sl->size-1; i++) {
        sl->squares[i] = sl->squares[i+1];
    }
    sl->size--;
}


void generate_pawn_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    uc piece, dest;
    uc curpos;
    int i, j;

    uc promotion_list[4] = {QUEEN, KNIGHT, ROOK, BISHOP};
    char capture_list[2] = {9,11};
    char incr;
    uc color_moving = 0;
    uc start_rank, penultimate_rank;

    if (!(pb->attrs & W_TO_MOVE)) {
        for (i = 0; i < 4; i++) {
            promotion_list[i] = promotion_list[i] | BLACK;
        }
        capture_list[0] = -9;
        capture_list[1] = -11;
        color_moving = BLACK;
        start_rank = 80;
        penultimate_rank = 30;
        incr = -10;
    } else {
        start_rank = 30;
        penultimate_rank = 80;
        incr = 10;
    }
    piece = pb->squares[s];
    curpos = s + incr;
    dest = pb->squares[curpos];
    if (dest == EMPTY) {
        if (s > penultimate_rank && s < (penultimate_rank + 10)) {
            for (i = 0; i< 4; i++) {
                MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, promotion_list[i], 0));
            }
        } else {
            MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, 0));
        }
        if (s > start_rank && s < (start_rank + 10)) {
            curpos = curpos + incr;
            dest = pb->squares[curpos];
            if (dest == EMPTY) {
                MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, MOVE_DOUBLE_PAWN));
            }
        }
    }

    for (i = 0; i < 2; i ++) {
        curpos = s + capture_list[i];
        if (pb->ep_target == curpos) {
            // regardless of which color is moving, EP capture takes the opposite color of it.
            MOVELIST_ADD(ml, create_move(s, curpos, piece, (piece ^ BLACK), 0, 0, MOVE_EN_PASSANT));
        } else {
            dest = pb->squares[curpos];
            if (dest != EMPTY && dest != OFF_BOARD && OPPOSITE_COLORS(piece,dest)) {
                if (s > penultimate_rank && s < (penultimate_rank + 10)) {
                    for (j = 0; j < 4; j ++) {
                        MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), promotion_list[j], 0));
                    }
                } else {
                    MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), 0, 0));
                }
            }
        }
    }
}

void generate_knight_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    int delta[8] = {-21, -19, -12, -8, 21, 19, 12, 8};
    uc piece, dest;
    uc curpos;
    int i;

    piece = pb->squares[s];
    for (i = 0; i < 8; i++) {
        curpos = s + delta[i];
        dest = pb->squares[curpos];
        if (dest == EMPTY) {
            MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, 0));
        } else {
            if (dest != OFF_BOARD && OPPOSITE_COLORS(dest, piece)) {
                MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), 0, 0));
            }
        }
    }
}

void generate_king_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    int delta[8] = {-1, 9, 10, 11, 1, -9, -10, -11};
    uc piece, dest;
    uc curpos;
    int i;

    piece = pb->squares[s];
    for (i=0; i<8; i++) {
        curpos = s + delta[i];
        dest = pb->squares[curpos];
        if (dest == EMPTY) {
            MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, 0));
        } else {
            if (dest != OFF_BOARD && OPPOSITE_COLORS(dest, piece)) {
                MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), 0, 0));
            }
        }
    }

    // Castling
    if (!(pb->attrs & BOARD_IN_CHECK)) {
        if (piece == WK && s == 25) {
            if (pb -> attrs & W_CASTLE_KING) {
                if (pb-> squares[26] == EMPTY && pb-> squares[27] == EMPTY && pb->squares[28]==WR) {
                    MOVELIST_ADD(ml, create_move(25, 27, WK, 0, 0, 0, MOVE_CASTLE));
                }
            }
            if (pb -> attrs & W_CASTLE_QUEEN) {
                if (pb->squares[21] == WR && pb-> squares[22] == EMPTY && pb-> squares[23] == EMPTY && pb->squares[24]==EMPTY) {
                    MOVELIST_ADD(ml, create_move(25, 23, WK, 0, 0, 0, MOVE_CASTLE));
                }
            }
        } else if (piece == BK && s == 95) {
            if (pb -> attrs & B_CASTLE_KING) {
                if (pb->squares[96] == EMPTY && pb-> squares[97] == EMPTY && pb->squares[98]==BR) {
                    MOVELIST_ADD(ml, create_move(95, 97, BK, 0, 0, 0, MOVE_CASTLE));
                }
            }
            if (pb -> attrs & B_CASTLE_QUEEN) {
                if (pb->squares[91] == BR && pb -> squares[92] == EMPTY && pb->squares[93] == EMPTY && pb->squares[94]==EMPTY) {
                    MOVELIST_ADD(ml, create_move(95, 93, BK, 0, 0, 0, MOVE_CASTLE));
                }
            }
        }
    }

}

void generate_directional_moves(const ChessBoard *pb, MoveList *ml, char velocity, uc s)
{
    uc curpos;
    uc piece, dest;

    piece = pb->squares[s];
    curpos = s + velocity;
    dest = pb->squares[curpos];
    while (dest == EMPTY) {
        MOVELIST_ADD(ml, create_move(s, curpos, piece, 0, 0, 0, 0));
        curpos = curpos + velocity;
        dest = pb->squares[curpos];
    }
    if (dest != OFF_BOARD && OPPOSITE_COLORS(dest,piece)) {
        MOVELIST_ADD(ml, create_move(s, curpos, piece, dest, piece_value(dest) - piece_value(piece), 0, 0));
    }

}

void generate_diagonal_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    generate_directional_moves(pb, ml, -9, s);
    generate_directional_moves(pb, ml, 9, s);
    generate_directional_moves(pb, ml, 11, s);
    generate_directional_moves(pb, ml, -11, s);
}

void generate_slide_moves(const struct ChessBoard *pb, MoveList *ml, uc s)
{
    generate_directional_moves(pb, ml, -1, s);
    generate_directional_moves(pb, ml, 1, s);
    generate_directional_moves(pb, ml, 10, s);
    generate_directional_moves(pb, ml, -10, s);
}


void generate_pinned_list(const struct ChessBoard *pb, SquareList *sl, bool for_defense)
{
    /*
     * If we are looking at defense, we are generating the list of pieces that are pinned.  That is, a friendly piece
     * blocking an enemy attacker from a friendly king.  If we are looking for attack, we are generating the list of
     * pieces which, if moved, could lead to discovered checks.  So this would be a friendly piece blocking a
     * friendly attacker from the enemy king.
     *
     * If white to move and for defense:
     *      White king, blocked by white pieces, attacked by black pieces
     * If white to move and for offense:
     *      Black king blocked by white pieces attacked by white pieces
     * If black to move and for defense:
     *      Black king, blocked by black pieces, attacked by white pieces
     * If black to move and for offense:
     *      White king, blocked by black pieces, attacked by black pieces
     */

    uc blocking_piece_color, defending_king, attacking_piece_color, king_position;
    uc attacking_bishop, attacking_rook, attacking_queen, velocity, cur_pos, cur_piece, pinning_pos;
    uc diagonal_velocity[4] = {-9, -11, 9, 11};
    uc slider_velocity[4] = {-1,10,1,-10};
    uc i;
    bool test_sliders = false, test_diagonals = false;

    SQUARELIST_CLEAR(sl);

    if (pb->attrs & W_TO_MOVE) {
        blocking_piece_color = WHITE;
        defending_king = for_defense ? WK : BK;
    } else {
        blocking_piece_color = BLACK;
        defending_king = for_defense ? BK : WK;
    }
    attacking_piece_color = (defending_king == WK) ? BLACK : WHITE;


    king_position = find_next_piece_location(pb, defending_king, 0);  // TODO replace with piece locations when built
    attacking_bishop = BISHOP | attacking_piece_color;
    attacking_rook = ROOK | attacking_piece_color;
    attacking_queen = QUEEN | attacking_piece_color;

    if (find_next_piece_location(pb, attacking_queen, 0) > 0) {
        test_diagonals = true;
        test_sliders = true;
    } else {
        if (find_next_piece_location(pb, attacking_bishop, 0) > 0) {
            test_diagonals = true;
        }
        if (find_next_piece_location(pb, attacking_rook, 0) > 0) {
            test_sliders = true;
        }
    }

    if (test_diagonals) {
        for (i=0;i<4;i++) {
            velocity = diagonal_velocity[i];
            cur_pos = king_position + velocity;
            cur_piece = pb->squares[cur_pos];
            while (cur_piece == EMPTY) {
                cur_pos = cur_pos + velocity;
                cur_piece = pb->squares[cur_pos];
            }
            if ((cur_piece & BLACK) == blocking_piece_color && !(cur_piece & OFF_BOARD)) {
                pinning_pos = cur_pos + velocity;
                cur_piece = pb->squares[pinning_pos];
                while (cur_piece == EMPTY) {
                    pinning_pos = pinning_pos + velocity;
                    cur_piece = pb->squares[pinning_pos];
                }
                if (cur_piece == attacking_queen || cur_piece == attacking_bishop) {
                    SQUARELIST_ADD(sl, cur_pos);
                }
            }
        }
    }

    if (test_sliders) {
        for (i=0;i<4;i++) {
            velocity = slider_velocity[i];
            cur_pos = king_position + velocity;
            cur_piece = pb->squares[cur_pos];
            while (cur_piece == EMPTY) {
                cur_pos = cur_pos + velocity;
                cur_piece = pb->squares[cur_pos];
            }
            if ((cur_piece & BLACK) == blocking_piece_color && !(cur_piece & OFF_BOARD)) {
                pinning_pos = cur_pos + velocity;
                cur_piece = pb->squares[pinning_pos];
                while (cur_piece == EMPTY) {
                    pinning_pos = pinning_pos + velocity;
                    cur_piece = pb->squares[pinning_pos];
                }
                if (cur_piece == attacking_queen || cur_piece == attacking_rook) {
                    SQUARELIST_ADD(sl, cur_pos);
                }
            }
        }
    }
}


int generate_move_list(const struct ChessBoard *pb, MoveList *ml)
{
    uc piece;
    int file, rank, i;
    uc color_moving;
    uc start, middle, end;
    struct ChessBoard tmp;
    Move check_flag = (Move)(MOVE_CHECK) << MOVE_FLAGS_SHIFT;
    Move castle_flag = (Move)(MOVE_CASTLE) << MOVE_FLAGS_SHIFT;

    MOVELIST_CLEAR(ml);
    color_moving = (pb->attrs & W_TO_MOVE) ? WHITE : BLACK;

    for (rank=20;rank< 100;rank=rank+10) {
        for (file=1; file<9; file++) {
            i = rank + file;
            piece = pb->squares[i];
            if (piece != EMPTY) {
                if (SAME_COLORS(piece, color_moving)) {
                    if (piece & PAWN) {
                        generate_pawn_moves(pb, ml, i);
                    } else if (piece & KNIGHT) {
                        generate_knight_moves(pb, ml, i);
                    } else if (piece & KING) {
                        generate_king_moves(pb, ml, i);
                    } else if (piece & BISHOP) {
                        generate_diagonal_moves(pb, ml, i);
                    } else if (piece & ROOK) {
                        generate_slide_moves(pb, ml, i);
                    } else if (piece & QUEEN) {
                        generate_diagonal_moves(pb, ml, i);
                        generate_slide_moves(pb, ml, i);
                    }
                }
            }
        }
    }
    for (i=ml->size-1; i>=0; i--) {
        // need to do this in descending order so the list_remove will only adjust moves we've already considered
        tmp = *pb;
        apply_move(&tmp, ml->moves[i]);
        // if the move leaves other side in check, add that to the move flag
        if (side_to_move_is_in_check(&tmp)) {
            ml->moves[i] = ml->moves[i] | check_flag;
        }

        // applying the move flips the W_TO_MOVE, so we need to flip it back to see if the move is illegal due to the side moving being in check
        tmp.attrs = tmp.attrs ^ W_TO_MOVE;
        if (side_to_move_is_in_check(&tmp)) {
            movelist_remove(ml,i);
        } else if (ml->moves[i] & castle_flag) {
            start = (uc) (ml->moves[i] & START);
            end = (uc) ((ml->moves[i] & END) >> END_SHIFT);
            middle = (start + end) / 2;
            tmp.squares[middle] = tmp.squares[end];
            tmp.squares[end] = EMPTY;
            if (side_to_move_is_in_check(&tmp)) {
                movelist_remove(ml, i);
            }
        }
    }
}