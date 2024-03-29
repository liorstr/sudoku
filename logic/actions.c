#include <stdio.h>
#include <stdlib.h>
#include "actions.h"
#include "../io/Serializer.h"
#include "../io/Printer.h"
#include "backtracking.h"
#include "ILP.h"
#include "LP.h"
#include "../MemoryError.h"


#define CANT_SAVE_UNSOLVABLE "Error: The current state of the puzzle " \
                             "is not solvable, so the board cannot be saved."
#define UNSOLVABLE_ERROR "Error: The current state of the puzzle " \
                         "is not solvable."
#define UNGUESSABLE_ERROR "Error: Could not make a guess based on the current " \
                          "state of the puzzle."
#define MAX_TRIALS_REACHED "Error: The maximum number of attempts to generate a " \
                          "puzzle has been reached. Could not generate puzzle."

#define DEFAULT_SIZE (3)
#define UNUSED(x) (void)(x)

/* Checks if the puzzle is finished in solve mode. If so, announces it to the
 * user, and switches to init mode. If the puzzle is finished but contains
 * errors, lets the user know that the the puzzle must be fixed before the game
 * can be won. */
void check_puzzle_finished(Game *game) {
    if (game->mode == solve_mode && game->board->empty_count == 0) {
        if (game->board->errors_count == 0) {
            announce_game_won();
            game->mode = init_mode;
        } else {
            announce_game_erroneous();
        }
    }
}

/* The different plays */

void play_solve(Command *command, Game *game) {
    Board *board = load_from_file(command->data.solve->path, command->error, solve_mode);
    if (is_valid(command)) {
        destroy_board(game->board);
        game->board = board;
        game->mode = solve_mode;
        clear_states_list(game->states);
        print(game);
    }
}

void play_edit(Command *command, Game *game) {
    Board *board;
    if (command->data.edit->from_file == true) {
        board = load_from_file(command->data.edit->path, command->error, edit_mode);
    } else {
        board = create_board(DEFAULT_SIZE, DEFAULT_SIZE);
    }

    if (is_valid(command)) {
        destroy_board(game->board);
        game->board = board;
        game->mode = edit_mode;
        clear_states_list(game->states);
        print(game);
    }
}

void play_mark_errors(Command *command, Game *game) {
    game->mark_errors = (bool) command->data.mark_errors->setting;
}

void play_print_board(Command *command, Game *game) {
    UNUSED(command);
    print(game);
}

void play_set(Command *command, Game *game) {
    add_new_move(game->states);
    make_change(game->board, game->states, command->data.set->row - 1,
                command->data.set->column - 1, command->data.set->value);
    print(game);

    /* Check if this is the last cell to be filled */
    check_puzzle_finished(game);
}

void play_validate(Command *command, Game *game) {
    UNUSED(command);

    if (is_board_solvable(game->board)) {
        announce_game_solvable();
    } else {
        announce_game_not_solvable();
    }
}

void play_guess(Command *command, Game *game) {
    add_new_move(game->states);
    if (!guess_solution(game->board, game->states, command->data.guess->threshold)) {
        invalidate(command, UNGUESSABLE_ERROR, execution_failure, false);
        delete_last_move(game->board, game->states);
        return;
    }
    print(game);
    check_puzzle_finished(game);
}

void play_generate(Command *command, Game *game) {
    add_new_move(game->states);
    if (!generate_puzzle(game->board, game->states, command->data.generate->num_to_fill,
                    command->data.generate->num_to_leave)) {
        invalidate(command, MAX_TRIALS_REACHED, execution_failure, false);
        delete_last_move(game->board, game->states);
        return;
    }

    print(game);
}

void play_undo(Command *command, Game *game) {
    Move *current_move = (Move*) get_current_item(game->states->moves);
    Change *change;

    UNUSED(command);

    reset_head(current_move->changes);
    if (is_empty(current_move->changes)) {
        announce_no_changes_made();
    }
    else {
        announce_changes_made();
        do {
            change = (Change*) get_current_item(current_move->changes);
            reset_change(game->board, change);
            print_change(change, true);
        }
        while (next(current_move->changes) == 0);
    }

    prev(game->states->moves);
    print(game);
}

void play_redo(Command *command, Game *game) {
    Move *current_move;
    Change *change;

    UNUSED(command);

    next(game->states->moves);
    current_move = (Move*) get_current_item(game->states->moves);

    reset_head(current_move->changes);
    if (is_empty(current_move->changes)) {
        announce_no_changes_made();
    } else {
        announce_changes_made();
        do {
            change = (Change *) get_current_item(current_move->changes);
            set_change(game->board, change);
            print_change(change, false);
        } while (next(current_move->changes) == 0);
    }
    print(game);
}

void play_save(Command *command, Game *game) {
    if (!is_board_solvable(game->board)) {
        invalidate(command, CANT_SAVE_UNSOLVABLE, execution_failure, false);
        return;
    }

    save_to_file(game, command->data.save->path, command->error);
}

void play_hint(Command *command, Game *game) {
    int actual_row = command->data.hint->row - 1;
    int actual_column = command->data.hint->column - 1;
    int hint = get_cell_solution(game->board, actual_row, actual_column);

    if (hint == ERROR_VALUE) {
        invalidate(command, UNSOLVABLE_ERROR, execution_failure, false);
        return;
    }

    announce_hint(hint);
}

void play_guess_hint(Command *command, Game *game) {
    int i;
    int actual_row = command->data.hint->row - 1;
    int actual_column = command->data.hint->column - 1;
    double *guesses = get_cell_guesses(game->board, actual_row, actual_column);

    if (guesses == NULL) {
        invalidate(command, UNGUESSABLE_ERROR, execution_failure, false);
        return;
    }

    announce_guesses_list();
    for (i=0; i < game->board->dim; i++) {
        if (guesses[i] != 0) {
            print_guess(i+1, guesses[i]);
        }
    }
    free(guesses);
}

void play_num_solutions(Command *command, Game *game) {
    UNUSED(command);

    announce_num_of_solutions(get_num_of_solutions(game->board));
}

void play_autofill(Command *command, Game *game) {
    int i, j, value;
    bool changes = false;

    Board *copy = get_board_copy(game->board);
    bool *marks = malloc(copy->dim * sizeof(bool));
    validate_memory_allocation("play_autofill", marks);

    UNUSED(command);
    add_new_move(game->states);

    for (i=0; i < copy->dim; i++) {
        for (j=0; j < copy->dim; j++) {
            if (!is_cell_empty(copy, i, j)) {
                continue;
            }

            value = get_obvious_value(copy, marks, i, j);
            if (value == ERROR_VALUE) {
                continue;
            }

            if (!changes) {
                changes = true;
                announce_changes_made();
            }

            make_change(game->board, game->states, i, j, value);
            print_autofill(i, j, value);
        }
    }
    destroy_board(copy);
    free(marks);

    if (!changes) {
        announce_no_changes_made();
    }
    print(game);

    /* Check if this is the last cell to be filled */
    check_puzzle_finished(game);
}

void play_reset(Command *command, Game *game) {
    Move *current_move;
    Change *change;

    UNUSED(command);

    while (has_prev(game->states->moves)) {
        current_move = (Move*) get_current_item(game->states->moves);
        if (!is_empty(current_move->changes)) {
            while (has_next(current_move->changes)) {
                next(current_move->changes);
            }

            do {
                change = (Change*) get_current_item(current_move->changes);
                reset_change(game->board, change);
            } while (prev(current_move->changes) == 0);
        }
        prev(game->states->moves);
    };

    print(game);
}

void play_exit_game(Command *command, Game *game) {
    UNUSED(command);

    game->over = true;
    announce_exit();
}
