#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "random.h"
#include "hash.h"


struct hashNode *TRANSPOSITION_TABLE;
struct hashNode *BB_TRANSPOSITION_TABLE;
long TRANSPOSITION_TABLE_SIZE = 1048799; //1048799; ////251611; // prime number;  NEEDS TO BE SAME FOR BOTH CACHES for simplicity of coding
bool MANUAL_SIZE_OVERRIDE_USED = false;

#ifndef NDEBUG
long DEBUG_TT_INSERTS;
long DEBUG_TT_PROBES;
#endif



unsigned long hash_whitetomove;
unsigned long hash_whitecastleking;
unsigned long hash_whitecastlequeen;
unsigned long hash_blackcastleking;
unsigned long hash_blackcastlequeen;

unsigned long bb_hash_whitetomove;
unsigned long bb_hash_whitecastleking;
unsigned long bb_hash_whitecastlequeen;
unsigned long bb_hash_blackcastleking;
unsigned long bb_hash_blackcastlequeen;

unsigned long hash_enpassanttarget[120];
unsigned long piece_hash[15][120];
unsigned long bb_piece_hash[15][64];
unsigned long bb_hash_enpassanttarget[64];




bool TT_init(long size)
{
    int rnd = 0; // runs from 0-789, which is the spot in our random array from random.h.
    uc i;
    unsigned long j;

#ifndef NDEBUG
    DEBUG_TT_INSERTS = 0;
    DEBUG_TT_PROBES = 0;
#endif

    assert (size == 0 || (!MANUAL_SIZE_OVERRIDE_USED) || size == TRANSPOSITION_TABLE_SIZE);

    if (size != 0) {
        TRANSPOSITION_TABLE_SIZE = size;
        MANUAL_SIZE_OVERRIDE_USED = true;
    }

    TRANSPOSITION_TABLE = (struct hashNode *) malloc (TRANSPOSITION_TABLE_SIZE * sizeof(struct hashNode));
    assert(TRANSPOSITION_TABLE); // TODO: Real error handling

    for (j = 0; j <= TRANSPOSITION_TABLE_SIZE; j++) {
        TRANSPOSITION_TABLE[j].hash = 0;
        MOVELIST_CLEAR(&TRANSPOSITION_TABLE[j].legal_moves);
    }

    // init the hash tables:
    for (i = 0; i < 120; i++) {
        if (arraypos_is_on_board(i)) {
            piece_hash[WP][i] = Random64[rnd++];
            piece_hash[WN][i] = Random64[rnd++];
            piece_hash[WB][i] = Random64[rnd++];
            piece_hash[WR][i] = Random64[rnd++];
            piece_hash[WQ][i] = Random64[rnd++];
            piece_hash[WK][i] = Random64[rnd++];
            piece_hash[BP][i] = Random64[rnd++];
            piece_hash[BN][i] = Random64[rnd++];
            piece_hash[BB][i] = Random64[rnd++];
            piece_hash[BR][i] = Random64[rnd++];
            piece_hash[BQ][i] = Random64[rnd++];
            piece_hash[BK][i] = Random64[rnd++];
        }
        if ((i >= 41 && i <= 48) || (i >= 71 && i <= 78)) {
            hash_enpassanttarget[i] = Random64[rnd++];
        }
    }
    hash_blackcastleking = Random64[rnd++];
    hash_blackcastlequeen = Random64[rnd++];
    hash_whitecastleking = Random64[rnd++];
    hash_whitecastlequeen = Random64[rnd++];
    hash_whitetomove = Random64[rnd++];

}

bool TT_init_bitboard(long size)
{
    int rnd = 0; // runs from 0-789, which is the spot in our random array from random.h.
    uc i;
    unsigned long j;

#ifndef NDEBUG
    DEBUG_TT_INSERTS = 0;
    DEBUG_TT_PROBES = 0;
#endif

    assert (size == 0 || (!MANUAL_SIZE_OVERRIDE_USED) || size == TRANSPOSITION_TABLE_SIZE);

    if (size != 0) {
        TRANSPOSITION_TABLE_SIZE = size;
        MANUAL_SIZE_OVERRIDE_USED = true;
    }


    BB_TRANSPOSITION_TABLE = (struct hashNode *) malloc (TRANSPOSITION_TABLE_SIZE * sizeof(struct hashNode));
    assert(BB_TRANSPOSITION_TABLE); // TODO: Real error handling

    for (j = 0; j <= TRANSPOSITION_TABLE_SIZE; j++) {
        BB_TRANSPOSITION_TABLE[j].hash = 0;
        MOVELIST_CLEAR(&BB_TRANSPOSITION_TABLE[j].legal_moves);
    }

    // init the hash tables:
    for (i = 0; i < 64; i++) {

        bb_piece_hash[WP][i] = Random64[rnd++];
        bb_piece_hash[WN][i] = Random64[rnd++];
        bb_piece_hash[WB][i] = Random64[rnd++];
        bb_piece_hash[WR][i] = Random64[rnd++];
        bb_piece_hash[WQ][i] = Random64[rnd++];
        bb_piece_hash[WK][i] = Random64[rnd++];
        bb_piece_hash[BP][i] = Random64[rnd++];
        bb_piece_hash[BN][i] = Random64[rnd++];
        bb_piece_hash[BB][i] = Random64[rnd++];
        bb_piece_hash[BR][i] = Random64[rnd++];
        bb_piece_hash[BQ][i] = Random64[rnd++];
        bb_piece_hash[BK][i] = Random64[rnd++];

        if ((i >= 16 && i <= 23) || (i >= 40 && i <= 47)) {
            bb_hash_enpassanttarget[i] = Random64[rnd++];
        }
    }
    bb_hash_blackcastleking = Random64[rnd++];
    bb_hash_blackcastlequeen = Random64[rnd++];
    bb_hash_whitecastleking = Random64[rnd++];
    bb_hash_whitecastlequeen = Random64[rnd++];
    bb_hash_whitetomove = Random64[rnd++];



}



bool TT_destroy() {

    if (TRANSPOSITION_TABLE) {
        free(TRANSPOSITION_TABLE);
    }
}

bool TT_destroy_bitboard() {

    if (BB_TRANSPOSITION_TABLE) {
        free(BB_TRANSPOSITION_TABLE);
    }
}

unsigned long compute_hash(const struct ChessBoard *pb) {
    unsigned long ret = 0;
    uc rank, file, i, attrs, piece;
    attrs = pb->attrs;

    if (attrs & W_TO_MOVE) {
        ret ^= hash_whitetomove;
    }
    if (attrs & W_CASTLE_KING) {
        ret ^= hash_whitecastleking;
    }
    if (attrs & W_CASTLE_QUEEN) {
        ret ^= hash_whitecastlequeen;
    }
    if (attrs & B_CASTLE_KING) {
        ret ^= hash_blackcastleking;
    }
    if (attrs & B_CASTLE_QUEEN) {
        ret ^= hash_blackcastlequeen;
    }
    if (pb->ep_target) {
        ret ^= hash_enpassanttarget[pb->ep_target];
    }


    for (rank = 20; rank < 100; rank = rank + 10) {
        for (file = 1; file < 9; file++) {
            i = rank + file;
            piece = pb->squares[i];
            if (piece != EMPTY) {
                ret ^= piece_hash[piece][i];
            }
        }
    }
    return (ret);
}

unsigned long compute_bitboard_hash(const struct bitChessBoard *pbb)
{
    unsigned long ret = 0;
    uc rank, file, i, piece;
    int castling, pos;
    castling = pbb->castling;
    uint_64 tmpmask;
    char *retstr;

    if (pbb->side_to_move == WHITE) {
        ret ^= bb_hash_whitetomove;
    }
    if (castling & W_CASTLE_KING) {
        ret ^= bb_hash_whitecastleking;
    }
    if (castling & W_CASTLE_QUEEN) {
        ret ^= bb_hash_whitecastlequeen;
    }
    if (castling & B_CASTLE_KING) {
        ret ^= bb_hash_blackcastleking;
    }
    if (castling & B_CASTLE_QUEEN) {
        ret ^= bb_hash_blackcastlequeen;
    }
    if (pbb->ep_target) {
        ret ^= bb_hash_enpassanttarget[pbb->ep_target];
    }

    // piece masks used for pieces are 1-6 (white pieces) and 9-14 (black pieces)
    for (piece=1; piece<=14; piece++) {
        if (piece<7 || piece>8) {
            tmpmask = pbb->piece_boards[piece];
            while (tmpmask) {
                pos = pop_lsb(&tmpmask);
                ret ^= bb_piece_hash[piece][pos];
            }
        }
    }

    return ret;


}

bool TT_insert(const struct ChessBoard *pb, const struct MoveList *ml)
{


    unsigned long hash, hash_modded;

#ifdef DISABLE_HASH
    return true;
#endif
    hash = pb->hash;

    hash_modded = hash % TRANSPOSITION_TABLE_SIZE;
    TRANSPOSITION_TABLE[hash_modded].hash = hash;
    TRANSPOSITION_TABLE[hash_modded].legal_moves = *ml;
#ifndef NDEBUG
    DEBUG_TT_INSERTS ++;
#endif
    return true;
}

bool TT_probe(const struct ChessBoard *pb, struct MoveList *ml)
{

    unsigned long hash, hash_modded;
#ifdef DISABLE_HASH
    return false;
#endif
    hash = pb->hash;
    hash_modded = hash % TRANSPOSITION_TABLE_SIZE;
    if (TRANSPOSITION_TABLE[hash_modded].hash == hash) {
        *ml = TRANSPOSITION_TABLE[hash_modded].legal_moves;
#ifndef NDEBUG
        DEBUG_TT_PROBES++;
#endif
        return true;
    }
    else {
        return false;
    }
}

bool TT_insert_bb(const struct bitChessBoard *pbb, const struct MoveList *ml)
{


    unsigned long hash, hash_modded;

#ifdef DISABLE_HASH
    return true;
#endif
    hash = pbb->hash;

    hash_modded = hash % TRANSPOSITION_TABLE_SIZE;
    BB_TRANSPOSITION_TABLE[hash_modded].hash = hash;
    BB_TRANSPOSITION_TABLE[hash_modded].legal_moves = *ml;
#ifndef NDEBUG
    DEBUG_TT_INSERTS ++;
#endif
    return true;
}

bool TT_probe_bb(const struct bitChessBoard *pbb, struct MoveList *ml)
{

    unsigned long hash, hash_modded;
#ifdef DISABLE_HASH
    return false;
#endif
    hash = pbb->hash;
    hash_modded = hash % TRANSPOSITION_TABLE_SIZE;
    if (BB_TRANSPOSITION_TABLE[hash_modded].hash == hash) {
        *ml = BB_TRANSPOSITION_TABLE[hash_modded].legal_moves;
#ifndef NDEBUG
        DEBUG_TT_PROBES++;
#endif
        return true;
    }
    else {
        return false;
    }
}