#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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

void movelist_remove(struct MoveList *ml, int position)
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

void squarelist_remove(struct SquareList *sl, int position)
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


bool square_in_list(const struct SquareList *sl, uc square)
{
    int i;
    for (i = 0; i < sl->size; i++) {
        if (sl->squares[i] == square) {
            return true;
        }
    }
    return false;
}


void generate_pawn_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    uc pawn_moving, occupant;
    uc curpos;
    int i, j;

    uc promotion_list[4] = {QUEEN, KNIGHT, ROOK, BISHOP};
    char capture_list[2];
    char incr;
    unsigned char enemy_king;
    unsigned char flags;
    uc color_moving = 0;
    uc start_rank, penultimate_rank;

    if (pb->attrs & W_TO_MOVE) {
        capture_list[0] = 9;
        capture_list[1] = 11;
        start_rank = 30;
        penultimate_rank = 80;
        incr = 10;
        enemy_king = BK;
    } else {
        for (i = 0; i < 4; i++) {
            promotion_list[i] = promotion_list[i] | BLACK;
        }
        capture_list[0] = -9;
        capture_list[1] = -11;
        color_moving = BLACK;
        start_rank = 80;
        penultimate_rank = 30;
        incr = -10;
        enemy_king = WK;
    }
    pawn_moving = pb->squares[s];
    curpos = s + incr;
    occupant = pb->squares[curpos];
    if (occupant == EMPTY) {
        if (s > penultimate_rank && s < (penultimate_rank + 10)) {
            for (i = 0; i< 4; i++) {
                MOVELIST_ADD(ml, create_move(s, curpos, pawn_moving, 0, 0, promotion_list[i], 0));
            }
        } else {
            flags = 0;
            if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                flags = MOVE_CHECK;
            }
            MOVELIST_ADD(ml, create_move(s, curpos, pawn_moving, 0, 0, 0, flags));
        }
        if (s > start_rank && s < (start_rank + 10)) {
            curpos = curpos + incr;
            occupant = pb->squares[curpos];
            if (occupant == EMPTY) {
                flags = MOVE_DOUBLE_PAWN;
                if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                    flags = flags | MOVE_CHECK;
                }
                MOVELIST_ADD(ml, create_move(s, curpos, pawn_moving, 0, 0, 0, flags));
            }
        }
    }

    for (i = 0; i < 2; i ++) {
        curpos = s + capture_list[i];
        if (pb->ep_target == curpos) {
            // regardless of which color is moving, EP capture takes the opposite color of it.
            flags = MOVE_EN_PASSANT;
            if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                flags = flags | MOVE_CHECK;
            }
            MOVELIST_ADD(ml, create_move(s, curpos, pawn_moving, (pawn_moving ^ BLACK), 0, 0, flags));
        } else {
            occupant = pb->squares[curpos];
            if (occupant != EMPTY && occupant != OFF_BOARD && OPPOSITE_COLORS(pawn_moving,occupant)) {
                if (s > penultimate_rank && s < (penultimate_rank + 10)) {
                    for (j = 0; j < 4; j ++) {
                        MOVELIST_ADD(ml, create_move(s, curpos, pawn_moving, occupant, piece_value(occupant) - piece_value(pawn_moving), promotion_list[j], 0));
                    }
                } else {
                    flags = 0;
                    if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                        flags = MOVE_CHECK;
                    }
                    MOVELIST_ADD(ml, create_move(s, curpos, pawn_moving, occupant, piece_value(occupant) - piece_value(pawn_moving), 0, flags));
                }
            }
        }
    }
}

void generate_knight_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    int delta[8] = {-21, -19, -12, -8, 21, 19, 12, 8};
    uc knight_moving, occupant;
    uc curpos, flags;
    uc destattack;
    int i,j;

    knight_moving = pb->squares[s];
    for (i = 0; i < 8; i++) {
        curpos = s + delta[i];
        occupant = pb->squares[curpos];
        if (occupant == EMPTY) {
            flags = 0;
            for (j = 0; j < 8; j++) {
                destattack = pb->squares[curpos + delta[j]];
                if ((destattack & KING) && (OPPOSITE_COLORS(destattack,knight_moving))) {
                    flags = MOVE_CHECK;
                    break;
                }
            }
            MOVELIST_ADD(ml, create_move(s, curpos, knight_moving, 0, 0, 0, flags));
        } else {
            if (occupant != OFF_BOARD && OPPOSITE_COLORS(occupant, knight_moving)) {
                flags = 0;
                for (j = 0; j < 8; j++) {
                    destattack = pb->squares[curpos + delta[j]];
                    if ((destattack & KING) && (OPPOSITE_COLORS(destattack,knight_moving))) {
                        flags = MOVE_CHECK;
                        break;
                    }
                }
                MOVELIST_ADD(ml, create_move(s, curpos, knight_moving, occupant, piece_value(occupant) - piece_value(knight_moving), 0, flags));
            }
        }
    }
}

void generate_king_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    int delta[8] = {-1, 9, 10, 11, 1, -9, -10, -11};
    uc king_moving, occupant;
    uc curpos;
    int i;

    king_moving = pb->squares[s];
    for (i=0; i<8; i++) {
        curpos = s + delta[i];
        occupant = pb->squares[curpos];
        if (occupant == EMPTY) {
            MOVELIST_ADD(ml, create_move(s, curpos, king_moving, 0, 0, 0, 0));
        } else {
            if (occupant != OFF_BOARD && OPPOSITE_COLORS(occupant, king_moving)) {
                MOVELIST_ADD(ml, create_move(s, curpos, king_moving, occupant, piece_value(occupant) - piece_value(king_moving), 0, 0));
            }
        }
    }

    // Castling
    if (!(pb->attrs & BOARD_IN_CHECK)) {
        if (king_moving == WK && s == 25) {
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
        } else if (king_moving == BK && s == 95) {
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

void generate_directional_moves(const ChessBoard *pb, MoveList *ml, char velocity, char perpendicular_velocity, uc s)
{
    // perpendicular velocity just saves me an if test, it is the direction 90 degrees from the velocity

    uc curpos, flags, testpos, testoccupant;
    uc piece_moving, occupant;
    char queen_delta[8] = {1, -1, 10, -10, -11, 11, 9, -9};
    char rookbishop_delta[3] = {velocity, perpendicular_velocity, (char)-1 * perpendicular_velocity};
    char *delta_to_use;
    char num_deltas, i, dir_to_test_for_check;

    piece_moving = pb->squares[s];

    if (piece_moving & QUEEN) {
        delta_to_use = queen_delta;
        num_deltas = 8;
    } else {
        /* Two ways for the move to be a check.  First, we take the piece that was blocking us from check, so
         * look straight ahead.  Then look perpendicular.  Cannot put the king into check behind us, else king
         * would already have been in check.
         */

        delta_to_use = rookbishop_delta;
        num_deltas = 3;
    }

    curpos = s + velocity;
    occupant = pb->squares[curpos];
    while (occupant == EMPTY) {
        flags = 0;
        for (i = 0; i < num_deltas; i++) {
            dir_to_test_for_check = delta_to_use[i];
            if (dir_to_test_for_check != velocity && dir_to_test_for_check != (-1 * velocity)) {
                // the rookbishop deltas already excluded -1 * velocity, but queen did not.
                // we do not check "velocity" for non-captures, because if the king were in check that way, king would
                // have already been in check.
                testpos = curpos + dir_to_test_for_check;
                testoccupant = pb->squares[testpos];
                while (testoccupant == EMPTY) {
                    testpos = testpos + dir_to_test_for_check;
                    testoccupant = pb->squares[testpos];
                }
                if ((testoccupant & KING) && (OPPOSITE_COLORS(testoccupant, piece_moving))) {
                    flags = MOVE_CHECK;
                    break;
                }
            }
        }
        MOVELIST_ADD(ml, create_move(s, curpos, piece_moving, 0, 0, 0, flags));
        curpos = curpos + velocity;
        occupant = pb->squares[curpos];
    }
    if (occupant != OFF_BOARD && OPPOSITE_COLORS(occupant,piece_moving)) {
        flags = 0;
        for (i = 0; i < num_deltas; i++) {
            dir_to_test_for_check = delta_to_use[i];
            if (dir_to_test_for_check != (-1 * velocity)) {
                // the rookbishop deltas already excluded -1 * velocity, but queen did not.
                testpos = curpos + dir_to_test_for_check;
                testoccupant = pb->squares[testpos];
                while (testoccupant == EMPTY) {
                    testpos = testpos + dir_to_test_for_check;
                    testoccupant = pb->squares[testpos];
                }
                if ((testoccupant & KING) && (OPPOSITE_COLORS(testoccupant, piece_moving))) {
                    flags = MOVE_CHECK;
                    break;
                }
            }
        }
        MOVELIST_ADD(ml, create_move(s, curpos, piece_moving, occupant, piece_value(occupant) - piece_value(piece_moving), 0, flags));
    }




}

void generate_diagonal_moves(const ChessBoard *pb, MoveList *ml, uc s)
{
    generate_directional_moves(pb, ml, -9, 11, s);
    generate_directional_moves(pb, ml, 9, 11, s);
    generate_directional_moves(pb, ml, 11, 9, s);
    generate_directional_moves(pb, ml, -11, 9, s);
}

void generate_slide_moves(const struct ChessBoard *pb, MoveList *ml, uc s)
{
    generate_directional_moves(pb, ml, -1, 10, s);
    generate_directional_moves(pb, ml, 1, 10, s);
    generate_directional_moves(pb, ml, 10, 1, s);
    generate_directional_moves(pb, ml, -10, 1, s);
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
    int i; // do not change this to a uc because uc's never become negative, and a for loop test will become an infinite loop below.
    uc piece;
    uc file, rank;
    uc color_moving;
    uc start, middle, end, piece_moving, piece_captured, promoted_to, flags;
    int capture_differential;
    struct ChessBoard tmp;
    struct SquareList pin_list, discovered_chk_list;
    Move m;
    Move check_flag = (Move)(MOVE_CHECK) << MOVE_FLAGS_SHIFT;
// debug code
    char *boardprint, *moveprint;

    bool currently_in_check;


    MOVELIST_CLEAR(ml);
    SQUARELIST_CLEAR(&pin_list);
    SQUARELIST_CLEAR(&discovered_chk_list);
    color_moving = (pb->attrs & W_TO_MOVE) ? WHITE : BLACK;
    currently_in_check = (pb->attrs & BOARD_IN_CHECK) ? true : false;

    generate_pinned_list(pb, &pin_list, true);
    generate_pinned_list(pb, &discovered_chk_list, false);

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
        m = ml->moves[i];
        parse_move(m, &start, &end, &piece_moving, &piece_captured, &capture_differential, &promoted_to, &flags);
        apply_move(&tmp, m);

        if (piece_moving & KING || square_in_list(&discovered_chk_list, start) || (promoted_to > 0) || (flags & MOVE_EN_PASSANT)) {
            // we tested for all other checks when we generated the moves
            if (side_to_move_is_in_check(&tmp)) {
                ml->moves[i] = m | check_flag;
            }
        }

        // debug section
/*
        parse_move(ml->moves[i], &start, &end, &piece_moving, &piece_captured, &capture_differential, &promoted_to, &flags);
        if ((flags & MOVE_CHECK) && !side_to_move_is_in_check(&tmp)) {
            boardprint = print_board(pb);
            moveprint = pretty_print_move(ml->moves[i]);
            printf("Error - Applying %s to board \n\n%s\n\n move is check, resulting board is not\n\n", moveprint, boardprint);
            ml->moves[i] = ml->moves[i] & (~check_flag);
            free(boardprint);
            free(moveprint);
        } else if ((!(flags & MOVE_CHECK)) && side_to_move_is_in_check(&tmp)) {
            boardprint = print_board(pb);
            moveprint = pretty_print_move(ml->moves[i]);
            printf("Error - Applying %s to board \n\n%s\n\n move is not check, resulting board is\n\n", moveprint, boardprint);
            ml->moves[i] = ml->moves[i] | check_flag;
            free(boardprint);
            free(moveprint);
        }
*/

        /*
         * Optimization - unless you are already in check, the only positions where you could move into check are king
         * moves, moves of pinned pieces, or en-passant captures (because could remove two pieces blocking king from check
         */

        if (currently_in_check || square_in_list(&pin_list, start) || (piece_moving & KING) || (flags & MOVE_EN_PASSANT)) {
            // applying the move flips the W_TO_MOVE, so we need to flip it back to see if the move is illegal due to the side moving being in check
            tmp.attrs = tmp.attrs ^ W_TO_MOVE;
            if (side_to_move_is_in_check(&tmp)) {
                movelist_remove(ml, i);
            } else if (flags & MOVE_CASTLE) {
                middle = (start + end) / 2;
                tmp.squares[middle] = tmp.squares[end];
                tmp.squares[end] = EMPTY;
                if (side_to_move_is_in_check(&tmp)) {
                    movelist_remove(ml, i);
                }
            }
        }
    }
}