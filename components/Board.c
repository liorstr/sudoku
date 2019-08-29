#include <stdlib.h>
#include <stdio.h>
#include "Board.h"
#include "../MemoryError.h"
#include "List.h"

struct BoardCell {
    int val;
    int row;
    int column;
    bool fixed;
    bool erroneous;
    List *neighbors;
    List *conflicting;
};


BoardCell* create_cell(int row, int column) {
    BoardCell *cell = malloc(sizeof(BoardCell));
    validate_memory_allocation("create_cell", cell);

    cell->val = CLEAR;
    cell->row = row;
    cell->column = column;
    cell->fixed = false;
    cell->erroneous = false;
    cell->neighbors = NULL;
    cell->conflicting = create_list();

    return cell;
}

void destroy_cell(BoardCell *cell) {
    if (cell->neighbors != NULL) {
        clear_list(cell->neighbors);
        destroy_list(cell->neighbors);
    }

    clear_list(cell->conflicting);
    destroy_list(cell->conflicting);
    free(cell);
}

BoardCell** create_row(int length, int index) {
    int i;
    BoardCell **row = malloc(length * sizeof(BoardCell*));
    validate_memory_allocation("create_row", row);

    for (i=0; i < length; i++) {
        row[i] = create_cell(index, i);
    }

    return row;
}

void destroy_row(BoardCell **row, int length) {
    int i;
    for (i=0; i < length; i++) {
        destroy_cell(row[i]);
    }

    free(row);
}

Board* create_board(int rows_in_block, int columns_in_block){
    int i, dim;
    BoardCell ***_cells_arr;

    Board *board = malloc(sizeof(Board));
    validate_memory_allocation("create_board", board);

    dim = rows_in_block * columns_in_block;
    board->num_of_rows_in_block = rows_in_block;
    board->num_of_columns_in_block = columns_in_block;
    board->dim = dim;
    board->solved = false;
    board->empty_count = dim * dim;
    board->errors_count = 0;

    _cells_arr = malloc(dim * sizeof(BoardCell**));
    validate_memory_allocation("create_board", board);

    for (i=0; i < dim; i++) {
        _cells_arr[i] = create_row(dim, i);
    }

    board->_cells_arr = _cells_arr;
    return board;
}

void destroy_board(Board *board){
    int i;

    if (board == NULL) {
        return;
    }
    
    for (i=0; i < board->dim; i++) {
        destroy_row(board->_cells_arr[i], board->dim);
    }

    free(board->_cells_arr);
    free(board);
}


/* Query functions */

bool is_cell_fixed(Board *board, int row, int column) {
    return board->_cells_arr[row][column]->fixed;
}

bool is_cell_empty(Board *board, int row, int column) {
    return board->_cells_arr[row][column]->val == CLEAR;
}

int get_cell_value(Board *board, int row, int column) {
    return board->_cells_arr[row][column]->val;
}

void get_block_indices(Board *board, int row, int column, int *r_start, int *r_end, int *c_start, int *c_end) {
    *r_start = row - (row % board->num_of_rows_in_block);
    *r_end = *r_start + board->num_of_rows_in_block;
    *c_start = column - (column % board->num_of_columns_in_block);
    *c_end = *c_start + board->num_of_columns_in_block;
}

List* generate_neighbors_list(Board *board, int row, int column) {
    int i, r_start, r_end, j, c_start, c_end;
    List *neighbors = create_list();

    /* row & column neighbors */
    for (i = 0; i < board->dim; i++) {
        if (i != column) {
            add(neighbors, board->_cells_arr[row][i]);
        }
        if (i != row) {
            add(neighbors, board->_cells_arr[i][column]);
        }
    }

    /* block neighbors */
    get_block_indices(board, row, column, &r_start, &r_end, &c_start, &c_end);
    for (i = r_start; i < r_end; i++) {
        for (j = c_start; j < c_end; j++) {
            if (i != row && j != column) {
                add(neighbors, board->_cells_arr[i][j]);
            }
        }
    }
    return neighbors;
}

List* get_neighbors(Board *board, int row, int column) {
    if (board->_cells_arr[row][column]->neighbors == NULL) {
        board->_cells_arr[row][column]->neighbors = generate_neighbors_list(board, row, column);
    }

    reset_head(board->_cells_arr[row][column]->neighbors);
    return board->_cells_arr[row][column]->neighbors;
}

List* get_conflicting(Board *board, int row, int column) {
    reset_head(board->_cells_arr[row][column]->conflicting);
    return board->_cells_arr[row][column]->conflicting;
}

/* Board manipulation functions */

/* TODO: remove this function, it's for debug purposes */
void print_cell_conflicts(Board *board, BoardCell *cell) {
    List *conflicting = get_conflicting(board, cell->row, cell->column);
    BoardCell *current;
    printf("=============================\n");
    printf("Board errors count: %d\n", board->errors_count);
    printf("(%d, %d): [", cell->column+1, cell->row+1);
    if (is_empty(conflicting)) {
        printf(" ---");
    } else {
        current = (BoardCell*) get_current_item(conflicting);
        printf("(%d, %d)", current->column+1, current->row+1);
        while (has_next(conflicting)) {
            next(conflicting);
            current = (BoardCell*) get_current_item(conflicting);
            printf(", (%d, %d)", current->column+1, current->row+1);
        }
    }
    printf("]\n");

}

void add_conflict(Board *board, BoardCell *cell, BoardCell *conflicting_cell) {
    /* fixed cells are never erroneous */
    if (cell->fixed) {
        return;
    }

    if (is_empty(cell->conflicting)) {
        cell->erroneous = true;
        board->errors_count++;
    }
    add(cell->conflicting, conflicting_cell);
}

void remove_conflict(Board *board, BoardCell *cell, BoardCell *conflicting_cell) {
    bool had_conflicts = !is_empty(cell->conflicting);

    delete(cell->conflicting, conflicting_cell);

    if (had_conflicts && is_empty(cell->conflicting)) {
        cell->erroneous = false;
        board->errors_count--;
    }
}

void set_cell_value(Board *board, int row, int column, int value) {
    BoardCell *neighbor;
    List *neighbors;

    /* no action needed (value wasn't changed) */
    if (board->_cells_arr[row][column]->val == value) {
        return;
    }

    neighbors = get_neighbors(board, row, column);

    do {
        neighbor = (BoardCell*) get_current_item(neighbors);
        if (neighbor->val == CLEAR) {
            /* conflicts are not relevant for clear cells */
        } else if (neighbor->val == value) {
            add_conflict(board, board->_cells_arr[row][column], neighbor);
            add_conflict(board, neighbor, board->_cells_arr[row][column]);

        } else if (neighbor->val == board->_cells_arr[row][column]->val) {
            remove_conflict(board, board->_cells_arr[row][column], neighbor);
            remove_conflict(board, neighbor, board->_cells_arr[row][column]);
        }
    } while (next(neighbors) == 0);

    if (value == CLEAR) {
        board->empty_count++;
    } else if (board->_cells_arr[row][column]->val == CLEAR) {
        board->empty_count--;
    }
    board->_cells_arr[row][column]->val = value;
}

bool fix_cell(Board *board, int row, int column) {
    BoardCell *conflicting_neighbor;
    List *conflicting = get_conflicting(board, row, column);

    /* validate there are no conflicting fixed neighbors */
    if (!is_empty(conflicting)) {
        do {
            conflicting_neighbor = (BoardCell*) get_current_item(conflicting);
            if (conflicting_neighbor->fixed == true) {
                return false;
            }
        } while (next(conflicting) == 0);
    }

    board->_cells_arr[row][column]->fixed = true;
    return true;
}

bool is_board_erroneous(Board *board){
    return (board->errors_count != 0);
}

bool is_cell_erroneous(Board *board, int row, int column){
    return board->_cells_arr[row][column]->erroneous;
}

Board* get_board_copy(Board *board){
    int i, j, val, N;
    Board *copy = create_board(board->num_of_rows_in_block, board->num_of_columns_in_block);
    N = board->dim;
    for (i=0 ; i<N ; i++){
        for (j=0 ; j<N ; j++){
            val = get_cell_value(board, i, j);
            set_cell_value(copy, i, j, val);
            if (is_cell_fixed(board, i, j)){
                fix_cell(copy, i, j);
            }
        }
    }

    if (board->solved){
        copy->solved = true;
    }

    return copy;
}

bool fix_non_empty_board_cells(Board *board){
    int row, column;

    for (row = 0; row < board->dim; row++){
        for (column = 0; column < board->dim; column++){
            if (!is_cell_empty(board, row, column)){
                if (!fix_cell(board, row, column)) {
                    return false;
                }
            }
        }
    }
    return true;
}