#pragma once

#include <stdbool.h>
#include "generate_moves.h"
#include "bitboard.h"


// Zobrist hashing / transposition table

extern unsigned long hash_whitetomove;
extern unsigned long hash_whitecastleking;
extern unsigned long hash_whitecastlequeen;
extern unsigned long hash_blackcastleking;
extern unsigned long hash_blackcastlequeen;

extern unsigned long bb_hash_whitetomove;
extern unsigned long bb_hash_whitecastleking;
extern unsigned long bb_hash_whitecastlequeen;
extern unsigned long bb_hash_blackcastleking;
extern unsigned long bb_hash_blackcastlequeen;

extern unsigned long hash_enpassanttarget[120];
extern unsigned long bb_hash_enpassanttarget[64];

/* pieces are 1=pawn, 2=knight, ... 6 = king, 9 = black pawn, .. 14 = black king.
 * so 0, 7, and 8 of the 15 are not used */

// TODO only compile in the one that we are using
extern unsigned long piece_hash[15][120];
extern unsigned long bb_piece_hash[15][64];

typedef struct hashNode {
    unsigned long hash;
    struct MoveList legal_moves;
    /*
     * short depth;
     * short best_score;
     * someScoreTypeEnum cache_score_type;
     * struct MoveList bestMoveSequence;
     */
} hashNode;

extern struct hashNode *TRANSPOSITION_TABLE;
extern struct hashNode *BB_TRANSPOSITION_TABLE;
extern long TRANSPOSITION_TABLE_SIZE;
extern bool MANUAL_SIZE_OVERRIDE_USED;

#ifndef NDEBUG
extern long DEBUG_TT_INSERTS;
extern long DEBUG_TT_PROBES;
#endif

bool TT_init(long size);
bool TT_init_bitboard(long size);
bool TT_destroy();
bool TT_destroy_bitboard();
bool TT_insert(const struct ChessBoard *pb, const struct MoveList *ml);
bool TT_insert_bb(const struct bitChessBoard *pbb, const struct MoveList *ml);
bool TT_probe(const struct ChessBoard *pb, struct MoveList *ml);
bool TT_probe_bb(const struct bitChessBoard *pbb, struct MoveList *ml);
unsigned long compute_hash(const struct ChessBoard *pb);
unsigned long compute_bitboard_hash(const struct bitChessBoard *pbb);