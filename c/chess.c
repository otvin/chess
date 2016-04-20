#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "hash.h"
#include "chess_constants.h"
#include "chessmove.h"
#include "chessboard.h"
#include "check_tables.h"
#include "chess.h"


int main()
{
    init_check_tables();
    TT_init(0);

    TT_destroy();
    return 0;
}
