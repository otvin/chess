#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>


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
                MOVELIST_ADD(ml, CREATE_MOVE(s, curpos, pawn_moving, 0, 0, promotion_list[i], 0));
            }
        } else {
            flags = 0;
            if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                flags = MOVE_CHECK;
            }
            MOVELIST_ADD(ml, CREATE_MOVE(s, curpos, pawn_moving, 0, 0, 0, flags));
        }
        if (s > start_rank && s < (start_rank + 10)) {
            curpos = curpos + incr;
            occupant = pb->squares[curpos];
            if (occupant == EMPTY) {
                flags = MOVE_DOUBLE_PAWN;
                if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                    flags = flags | MOVE_CHECK;
                }
                MOVELIST_ADD(ml, CREATE_MOVE(s, curpos, pawn_moving, 0, 0, 0, flags));
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
            MOVELIST_ADD(ml, CREATE_MOVE(s, curpos, pawn_moving, (pawn_moving ^ BLACK), 0, 0, flags));
        } else {
            occupant = pb->squares[curpos];
            if (occupant != EMPTY && occupant != OFF_BOARD && OPPOSITE_COLORS(pawn_moving,occupant)) {
                if (s > penultimate_rank && s < (penultimate_rank + 10)) {
                    for (j = 0; j < 4; j ++) {
                        MOVELIST_ADD(ml, CREATE_MOVE(s, curpos, pawn_moving, occupant, piece_value(occupant) - piece_value(pawn_moving), promotion_list[j], 0));
                    }
                } else {
                    flags = 0;
                    if (pb->squares[curpos + capture_list[0]] == enemy_king || pb->squares[curpos + capture_list[1]] == enemy_king) {
                        flags = MOVE_CHECK;
                    }
                    MOVELIST_ADD(ml, CREATE_MOVE(s, curpos, pawn_moving, occupant, piece_value(occupant) - piece_value(pawn_moving), 0, flags));
                }
            }
        }
    }
}



bool test_for_check_after_castle(const struct ChessBoard *pb, char rook_pos, char direction1, char direction2, uc enemy_king)
{
    // I could figure out the directions and the enemy king from the rook position, but faster execution if the caller hard codes
    char curpos;
    uc occupant;

    curpos = rook_pos + direction1;
    occupant = pb->squares[curpos];
    while (occupant == EMPTY) {
        curpos = curpos + direction1;
        occupant = pb->squares[curpos];
    }
    if (occupant == enemy_king) {
        return true;
    }
    curpos = rook_pos + direction2;
    occupant = pb->squares[curpos];
    while (occupant == EMPTY) {
        curpos = curpos + direction2;
        occupant = pb->squares[curpos];
    }
    if (occupant == enemy_king) {
        return true;
    }
    return false;

}



void generate_pinned_list(const struct ChessBoard *pb, SquareList *sl, bool for_defense, uc kingpos, bool include_sliders, bool include_diags)
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

    uc blocking_piece_color, defending_king, attacking_piece_color;
    uc attacking_bishop, attacking_rook, attacking_queen, velocity, cur_pos, cur_piece, pinning_pos;
    uc diagonal_velocity[4] = {-9, -11, 9, 11};
    uc slider_velocity[4] = {-1,10,1,-10};
    uc i;

    SQUARELIST_CLEAR(sl);

    if (pb->attrs & W_TO_MOVE) {
        blocking_piece_color = WHITE;
        defending_king = for_defense ? WK : BK;
    } else {
        blocking_piece_color = BLACK;
        defending_king = for_defense ? BK : WK;
    }
    attacking_piece_color = (defending_king == WK) ? BLACK : WHITE;

    attacking_bishop = BISHOP | attacking_piece_color;
    attacking_rook = ROOK | attacking_piece_color;
    attacking_queen = QUEEN | attacking_piece_color;

    if (include_diags) {
        for (i=0;i<4;i++) {
            velocity = diagonal_velocity[i];
            cur_pos = kingpos + velocity;
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

    if (include_sliders) {
        for (i=0;i<4;i++) {
            velocity = slider_velocity[i];
            cur_pos = kingpos + velocity;
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


    // Pieces in order:  Pawn = 1, knight = 2, bishop = 3, rook = 4, queen = 5, king = 6

    // move rewrite inspired by Tom Kerrigan and TSCP.
    char deltas[7][8] = {
            {0, 0, 0, 0, 0, 0, 0, 0}, // empty squares have no moves
            {0, 0, 0, 0, 0, 0, 0, 0},  // Pawns have special moves
            {-21, -19, -12, -8, 21, 19, 12, 8}, // Knight moves
            {-9, -11, 9, 11, 0, 0, 0, 0}, // Bishop
            {1, -1, 10, -10, 0, 0, 0, 0}, // Rook
            {-9, -11, 9, 11, 1, -1, 10, -10}, // Queen
            {-9, -11, 9, 11, 1, -1, 10, -10} // King
    };
    bool piece_slides[7] = {false, false, false, true, true, true, false};
    char num_deltas [7] = {0, 0, 8, 4, 4, 8, 8};
    int i; // do not change this to a uc because uc's never become negative, and a for loop test will become an infinite loop below.
    struct ChessBoard tmp;
    struct SquareList pin_list, discovered_check_list;
    bool has_friendly_slider = false, has_friendly_diag = false;
    bool has_opponent_slider = false, has_opponent_diag = false;
    bool currently_in_check;
    bool can_castle_queen, can_castle_king;
    bool move_removed;
    uc color_moving;
    char rank, file, delta, piece, p7, flags;
    uc j, occupant, curpos, friendly_kingpos, enemy_kingpos, attrs, q, nextoccupant, searchpos;
    Move m;
    Move check_flag = (Move)(MOVE_CHECK) << MOVE_FLAGS_SHIFT;
    uc start, end, middle, promoted_to, piece_moving, enemy_king;
    bool found_check, removed_move;


    MOVELIST_CLEAR(ml);
    SQUARELIST_CLEAR(&pin_list);
    SQUARELIST_CLEAR(&discovered_check_list);
    attrs = pb->attrs;
    if (attrs & W_TO_MOVE) {
        color_moving = WHITE;
        can_castle_queen = attrs & W_CASTLE_QUEEN;
        can_castle_king = attrs & W_CASTLE_KING;
        enemy_king = BK;
    } else {
        color_moving = BLACK;
        can_castle_queen = attrs & B_CASTLE_QUEEN;
        can_castle_king = attrs & B_CASTLE_KING;
        enemy_king = WK;
    }

    currently_in_check = (attrs & BOARD_IN_CHECK) ? true : false;


    for (rank = 20; rank < 100; rank += 10) {
        for (file = 1; file < 9; file++) {
            i = rank + file;
            if ((piece = pb->squares[i]) != EMPTY) {
                p7 = PIECE_BITS(piece);
                if OPPOSITE_COLORS(piece, color_moving) {
                    switch(p7) {
                        case(KING):
                            enemy_kingpos = i;
                            break;
                        case(QUEEN):
                            has_opponent_slider = true;
                            has_opponent_diag = true;
                            break;
                        case(ROOK):
                            has_opponent_slider = true;
                            break;
                        case(BISHOP):
                            has_opponent_diag = true;
                            break;
                    }
                } else {
                    if (p7 != PAWN) {
                        switch(p7) {
                            case (QUEEN):
                                has_friendly_slider = true;
                                has_friendly_diag = true;
                                break;
                            case (ROOK):
                                has_friendly_slider = true;
                                break;
                            case (BISHOP):
                                has_friendly_diag = true;
                                break;
                        }
                        for (j=0; j<num_deltas[p7]; j++) {
                            curpos = i;
                            while(true) {
                                occupant = pb->squares[curpos += deltas[p7][j]];
                                if (occupant) {
                                    if (occupant == OFF_BOARD || SAME_COLORS(piece, occupant)) {
                                        break;
                                    } else {
                                        found_check = false;
                                        // will the resulting move leave enemy in check?
                                        for (q = 0; q < num_deltas[p7] && !found_check; q++) {
                                            if (deltas[p7][q] != (-1 *
                                                                  deltas[p7][j])) {  // king cannot be in check moving back the way we got here
                                                searchpos = curpos; // curpos is destination of move, searchpos is candidate square for enemy king;
                                                while (true) {
                                                    nextoccupant = pb->squares[searchpos += deltas[p7][q]];
                                                    if (nextoccupant != EMPTY) {
                                                        if (nextoccupant == enemy_king) {
                                                            found_check = true;
                                                        }
                                                        break;
                                                    }
                                                    if (!piece_slides[p7]) {
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        if ((!found_check) || p7 != KING) {
                                            MOVELIST_ADD(ml, CREATE_MOVE(i, curpos, piece, occupant, piece_value(occupant) - piece_value(piece), 0, found_check ? MOVE_CHECK : 0));
                                        }
                                        break;
                                    }
                                }
                                found_check = false;
                                // will the resulting move leave enemy in check?
                                for (q = 0; q < num_deltas[p7] && !found_check; q++) {
                                    if (deltas[p7][q] != (-1 *
                                                          deltas[p7][j])) {  // king cannot be in check moving back the way we got here
                                        searchpos = curpos; // curpos is destination of move, searchpos is candidate square for enemy king;
                                        while (true) {
                                            nextoccupant = pb->squares[searchpos += deltas[p7][q]];
                                            if (nextoccupant != EMPTY) {
                                                if (nextoccupant == enemy_king) {
                                                    found_check = true;
                                                }
                                                break;
                                            }
                                            if (!piece_slides[p7]) {
                                                break;
                                            }
                                        }
                                    }
                                }
                                if ((!found_check) || p7 != KING) {
                                    MOVELIST_ADD(ml, CREATE_MOVE(i, curpos, piece, 0, 0, 0, found_check ? MOVE_CHECK : 0));
                                }
                                if (!piece_slides[p7]) {
                                    break;
                                }
                            }
                        }
                        if (p7 == KING) {
                            friendly_kingpos = i;
                            if (!currently_in_check) {
                                if (can_castle_king && (PIECE_BITS(pb->squares[i + 3]) == ROOK)) {
                                    // start square must be the home square else the attribute would be false and similarly rook must be on its home square
                                    assert(i == 25 || i == 95);
                                    if (pb->squares[i + 1] == EMPTY && pb->squares[i + 2] == EMPTY) {
                                        MOVELIST_ADD(ml, CREATE_MOVE(i, i + 2, piece, 0, 0, 0, test_for_check_after_castle(pb, i+1, -1, i == 95 ? -10 : 10, enemy_king) ? MOVE_CHECK | MOVE_CASTLE : MOVE_CASTLE));
                                    }
                                }
                                if (can_castle_queen && (PIECE_BITS(pb->squares[i - 4]) == ROOK)) {
                                    assert(i == 25 || i == 95);
                                    if (pb->squares[i - 1] == EMPTY && pb->squares[i - 2] == EMPTY &&
                                        pb->squares[i - 3] == EMPTY) {
                                        MOVELIST_ADD(ml, CREATE_MOVE(i, i - 2, piece, 0, 0, 0, test_for_check_after_castle(pb, i-1, 1, i == 95 ? -10 : 10, enemy_king) ? MOVE_CHECK | MOVE_CASTLE : MOVE_CASTLE));
                                    }
                                }
                            }
                        }
                    } else {
                        generate_pawn_moves(pb, ml, i);
                    }
                }
            }
        }
    }

    generate_pinned_list(pb, &pin_list, true, friendly_kingpos, has_opponent_slider, has_opponent_diag);
    generate_pinned_list(pb, &discovered_check_list, false, enemy_kingpos, has_friendly_slider, has_friendly_diag);


    for (i=ml->size-1; i>=0; i--) {
        tmp = *pb;
        m = ml->moves[i];
        start = GET_START(m);
        promoted_to = GET_PROMOTED_TO(m);
        flags = GET_FLAGS(m);
        piece_moving = GET_PIECE_MOVING(m);
        apply_move(&tmp,m);



        // Unless you are already in check, the only positions where you could move into check are king moves, moves of
        // pinned pieces, or en-passant captures (because you could remove two pieces blocking king from check)
        removed_move = false;
        if (currently_in_check || square_in_list(&pin_list, start) || (PIECE_BITS(piece_moving) == KING) || (flags & MOVE_EN_PASSANT)) {
            // applying the move flips the W_TO_MOVE, so we need to flip it back to see if the move is illegal due to the side moving being in check
            tmp.attrs ^= W_TO_MOVE;
            if (side_to_move_is_in_check(&tmp, 0)) {
                movelist_remove(ml, i);
                removed_move = true;
            } else if (flags & MOVE_CASTLE) {
                end = GET_END(m);
                middle = (start + end) / 2;
                tmp.squares[middle] = tmp.squares[end];
                tmp.squares[end] = EMPTY;
                if (side_to_move_is_in_check(&tmp, 0)) {
                    movelist_remove(ml, i);
                    removed_move = true;
                }
            }
            tmp.attrs ^= W_TO_MOVE;
        }
        if (!removed_move) {
            // if the move is legal, and it is one of these few cases where we didn't compute check when we made the move, compute it now.
            if (square_in_list(&discovered_check_list, start) || (promoted_to > 0) || (flags & MOVE_EN_PASSANT)) {
                // we tested for all other checks when we generated the moves
                if (side_to_move_is_in_check(&tmp, enemy_kingpos)) {
                    ml->moves[i] = m | check_flag;
                }
            }
        }

    }
}