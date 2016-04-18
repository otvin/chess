#pragma once

#include "generate_moves.h"
#include <stdbool.h>

// Zobrist hashing / transposition table


extern unsigned long hash_whitetomove;
extern unsigned long hash_whitecastleking;
extern unsigned long hash_whitecastlequeen;
extern unsigned long hash_blackcastleking;
extern unsigned long hash_blackcastlequeen;

extern unsigned long hash_enpassanttarget[120];
extern unsigned long hash_whitep[120];
extern unsigned long hash_blackp[120];
extern unsigned long hash_whiten[120];
extern unsigned long hash_blackn[120];
extern unsigned long hash_whiteb[120];
extern unsigned long hash_blackb[120];
extern unsigned long hash_whiter[120];
extern unsigned long hash_blackr[120];
extern unsigned long hash_whiteq[120];
extern unsigned long hash_blackq[120];
extern unsigned long hash_whitek[120];
extern unsigned long hash_blackk[120];

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
extern long TRANSPOSITION_TABLE_SIZE;
extern long DEBUG_TT_INSERTS;
extern long DEBUG_TT_PROBES;

bool TT_init(long size);
bool TT_destroy();
bool TT_insert(const struct ChessBoard *pb, const struct MoveList *ml);
bool TT_probe(const struct ChessBoard *pb, struct MoveList *ml);
unsigned long hashsquare_for_bitflag_piece(uc piece, uc pos);
unsigned long compute_hash(const struct ChessBoard *pb);