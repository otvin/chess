#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "chess_constants.h"
#include "chessmove.h"
#include "chessboard.h"
#include "check_tables.h"
#include "chess.h"


int main()
{
    init_check_tables();

    return 0;
}
