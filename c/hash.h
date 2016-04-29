#pragma once

#include <stdbool.h>
#include "generate_moves.h"
#include "bitboard.h"


// Zobrist hashing / transposition table

extern uint_64 hash_whitetomove;
extern uint_64 hash_whitecastleking;
extern uint_64 hash_whitecastlequeen;
extern uint_64 hash_blackcastleking;
extern uint_64 hash_blackcastlequeen;

extern uint_64 bb_hash_castling[16];

extern uint_64 bb_hash_whitetomove;


extern uint_64 hash_enpassanttarget[120];
extern uint_64 bb_hash_enpassanttarget[64];

/* pieces are 1=pawn, 2=knight, ... 6 = king, 9 = black pawn, .. 14 = black king.
 * so 0, 7, and 8 of the 15 are not used */

// TODO only compile in the one that we are using
extern uint_64 piece_hash[15][120];
extern uint_64 bb_piece_hash[15][64];

typedef struct hashNode {
	uint_64 hash;
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

void TT_init(long size);
void TT_init_bitboard(long size);
void TT_destroy();
void TT_destroy_bitboard();
bool TT_insert(const struct ChessBoard *pb, const struct MoveList *ml);
bool TT_insert_bb(const struct bitChessBoard *pbb, const struct MoveList *ml);
bool TT_probe(const struct ChessBoard *pb, struct MoveList *ml);
bool TT_probe_bb(const struct bitChessBoard *pbb, struct MoveList *ml);
unsigned long compute_hash(const struct ChessBoard *pb);
unsigned long compute_bitboard_hash(const struct bitChessBoard *pbb);