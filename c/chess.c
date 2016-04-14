#include "chess.h"


int main() {

    struct ChessBoard *pBoard;
    struct MoveList *pList;
    Move m;
    char *movestr;

    pBoard = new_board();
    set_start_position(pBoard);
    print_board(pBoard);
    printf("\n");

    pList = new_empty_move_list();
    m = create_move(21,31,WR,0,0,0,0);
    movestr = pretty_print_move(m);
    printf("%s\n", movestr);
    free(movestr);
    add_move_to_list(pList, m);

    m = create_move(95,97,BK,0,0,0,MOVE_CASTLE);
    movestr = pretty_print_move(m);
    printf("%s\n", movestr);
    free(movestr);
    add_move_to_list(pList, m);

    m = create_move(34,56,WB,BP,0,0,0);
    movestr = pretty_print_move(m);
    printf("\n%s", movestr);
    free(movestr);
    add_move_to_list(pList, m);

    printf("\n\n\nPrinting List\n");
    print_move_list(*pList);
    printf("\n");


    delete_moves_in_list(pList);
    free(pList);
    free(pBoard);
    return(0);
}
