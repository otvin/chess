#include <stdio.h>
#include <stdlib.h>

#include "random.h"
#include "hash.h"

struct hashNode *TRANSPOSITION_TABLE;
long TRANSPOSITION_TABLE_SIZE;
long DEBUG_TT_INSERTS;
long DEBUG_TT_PROBES;




unsigned long hash_whitetomove;
unsigned long hash_whitecastleking;
unsigned long hash_whitecastlequeen;
unsigned long hash_blackcastleking;
unsigned long hash_blackcastlequeen;

unsigned long hash_enpassanttarget[120];
unsigned long hash_whitep[120];
unsigned long hash_blackp[120];
unsigned long hash_whiten[120];
unsigned long hash_blackn[120];
unsigned long hash_whiteb[120];
unsigned long hash_blackb[120];
unsigned long hash_whiter[120];
unsigned long hash_blackr[120];
unsigned long hash_whiteq[120];
unsigned long hash_blackq[120];
unsigned long hash_whitek[120];
unsigned long hash_blackk[120];




bool TT_init(long size)
{
    int rnd = 0; // runs from 0-789, which is the spot in our random array from random.h.
    uc i;
    unsigned long j;

    DEBUG_TT_INSERTS = 0;  // TODO only set these if in debug mode of some form
    DEBUG_TT_PROBES = 0;

    if (size == 0) {
        TRANSPOSITION_TABLE_SIZE = 1048799; ////251611; // prime number
    } else {
        TRANSPOSITION_TABLE_SIZE = size;
    }

    TRANSPOSITION_TABLE = (struct hashNode *) malloc (TRANSPOSITION_TABLE_SIZE * sizeof(struct hashNode));
    if (!TRANSPOSITION_TABLE) {
        printf("barf!"); // TODO - real error handling
    }

    else {

        for (j = 0; j <= TRANSPOSITION_TABLE_SIZE; j++) {
            TRANSPOSITION_TABLE[j].hash = 0;
            MOVELIST_CLEAR(&TRANSPOSITION_TABLE[j].legal_moves);
        }

        // init the hash tables:
        for (i = 0; i < 120; i++) {
            if (arraypos_is_on_board(i)) {
                hash_whitep[i] = Random64[rnd++];
                hash_whiten[i] = Random64[rnd++];
                hash_whiteb[i] = Random64[rnd++];
                hash_whiter[i] = Random64[rnd++];
                hash_whiteq[i] = Random64[rnd++];
                hash_whitek[i] = Random64[rnd++];
                hash_blackp[i] = Random64[rnd++];
                hash_blackn[i] = Random64[rnd++];
                hash_blackb[i] = Random64[rnd++];
                hash_blackr[i] = Random64[rnd++];
                hash_blackq[i] = Random64[rnd++];
                hash_blackk[i] = Random64[rnd++];
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
}

bool TT_destroy() {

    if (TRANSPOSITION_TABLE) {
        free(TRANSPOSITION_TABLE);
    }
}

unsigned long hashsquare_for_bitflag_piece(uc piece, uc pos) {

    switch(piece) {
        case(WP):
            return hash_whitep[pos];
        case(WN):
            return hash_whiten[pos];
        case(WB):
            return hash_whiteb[pos];
        case(WR):
            return hash_whiter[pos];
        case(WQ):
            return hash_whiteq[pos];
        case(WK):
            return hash_whitek[pos];
        case(BP):
            return hash_blackp[pos];
        case(BN):
            return hash_blackn[pos];
        case(BB):
            return hash_blackb[pos];
        case(BR):
            return hash_blackr[pos];
        case(BQ):
            return hash_blackq[pos];
        case(BK):
            return hash_blackk[pos];
        default:
            return 0;
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
                ret ^= hashsquare_for_bitflag_piece(piece, i);
            }
        }
    }
    return (ret);
}

bool TT_insert(const struct ChessBoard *pb, const struct MoveList *ml)
{

    unsigned long hash, hash_modded;
    hash = pb->hash;

    hash_modded = hash % TRANSPOSITION_TABLE_SIZE;
    TRANSPOSITION_TABLE[hash_modded].hash = hash;
    TRANSPOSITION_TABLE[hash_modded].legal_moves = *ml;
    //DEBUG_TT_INSERTS ++; // TODO - only do this in debug mode
    return true;
}

bool TT_probe(const struct ChessBoard *pb, struct MoveList *ml)
{

    unsigned long hash, hash_modded;
    hash = pb->hash;
    hash_modded = hash % TRANSPOSITION_TABLE_SIZE;
    if (TRANSPOSITION_TABLE[hash_modded].hash == hash) {
        *ml = TRANSPOSITION_TABLE[hash_modded].legal_moves;
        //DEBUG_TT_PROBES++; // TODO - only do this in debug mode
        return true;
    }
    else {
        return false;
    }
}