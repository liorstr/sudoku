#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "Parser.h"
#include "validators.h"
#include "../logic/actions.h"
#include "../MemoryError.h"

#define MAX_ARGS 4
#define WHITESPACE " \t\r\n"

#define INT_BASE 10
#define ERROR_VALUE (-1)

#define INVALID_COMMAND_INIT_ERROR "Error: invalid command. Available commands are: solve, edit, exit."
#define INVALID_COMMAND_EDIT_ERROR "Error: invalid command. Available commands are: solve, edit, print_board, set, " \
                                   "validate, generate, undo, redo, save, num_solutions, reset, exit."
#define INVALID_COMMAND_SOLVE_ERROR "Error: invalid command. Available commands are: solve, edit, mark_errors, " \
                                   "print_board, set, validate, guess, undo, redo, save, hint, guess_hint, " \
                                   "num_solutions, autofill, reset, exit."
#define TOO_MANY_ARGS_ERROR "Error: too many arguments were given."
#define TOO_FEW_ARGS_ERROR "Error: not enough arguments were given."

#define MAX_ERROR_MESSAGE_LEN 1024

#define UNUSED(x) (void)(x)

/* Asserts the number of arguments that was entered by the user is correct. */
void assert_num_of_args(Command *command, int min_expected, int max_expected, int actual) {
    char *error_message, *error_type;

    if (actual > max_expected) {
        error_type = TOO_MANY_ARGS_ERROR;
    }
    else if (actual < min_expected) {
        error_type = TOO_FEW_ARGS_ERROR;
    } else {
        return;  /* valid */
    }

    error_message = calloc(MAX_ERROR_MESSAGE_LEN, sizeof(char));
    validate_memory_allocation("assert_num_of_args", error_message);

    sprintf(error_message, "%s Correct command format: %s", error_type, command->format);
    invalidate(command, error_message, invalid_num_of_args, true);
}


/* Parsers for specific types of args */

/* Parses an argument that should be an int. */
void parse_int_arg(char *token, int *p_int) {
    int num;
    char *end_p;

    errno = 0;
    num = strtol(token, &end_p, INT_BASE);

    /* check if token is a valid positive integer */
    if (num >= 0 && token != end_p && *end_p == '\0' && errno == 0) {
        *p_int = num;
    } else {
        *p_int = ERROR_VALUE;
    }
}

/* Parses an argument that should be a double. */
void parse_double_arg(char *token, double *p_double) {
    double num;
    char *end_p;

    errno = 0;
    num = strtod(token, &end_p);

    /* check if token is a valid positive double */
    if (num >= 0 && token != end_p && *end_p == '\0' && errno == 0) {
        *p_double = num;
    } else {
        *p_double = ERROR_VALUE;
    }
}

/* Parses an argument that should be a boolean. */
void parse_bool_arg(char *token, int *p_bool) {
    if (strcmp(token, "0") == 0) {
        *p_bool = 0;
    } else if (strcmp(token, "1") == 0) {
        *p_bool = 1;
    } else {
        /* invalid value for boolean field */
        *p_bool = ERROR_VALUE;
    }
}


/* Command-specific arg parsers */

/* Parses the arguments of a solve command. */
void solve_args_parser(Command *self, char **args, int num_of_args) {
    SolveCommand *data = malloc(sizeof(SolveCommand));
    validate_memory_allocation("solve_args_parser", data);

    assert_num_of_args(self, SOLVE_ARGS, SOLVE_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    data->path = (char*) calloc(strlen(args[0]) + 1, sizeof(char));
    validate_memory_allocation("solve_args_parser", data->path);
    strcpy(data->path, args[0]);
    self->data.solve = data;
}

/* Parses the arguments of an edit command. */
void edit_args_parser(Command *self, char **args, int num_of_args) {
    EditCommand *data = malloc(sizeof(EditCommand));
    validate_memory_allocation("edit_args_parser", data);

    assert_num_of_args(self, EDIT_MIN_ARGS, EDIT_MAX_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    if (num_of_args == EDIT_MAX_ARGS) {
        data->from_file = true;
        data->path = calloc(strlen(args[0]) + 1, sizeof(char));
        validate_memory_allocation("edit_args_parser", data);
        strcpy(data->path, args[0]);

    } else if (num_of_args == EDIT_MIN_ARGS) {
        data->from_file = false;
        data->path = NULL;
    }

    self->data.edit = data;
}

/* Parses the arguments of a mark_errors command. */
void mark_errors_args_parser(Command *self, char **args, int num_of_args) {
    MarkErrorsCommand *data = malloc(sizeof(MarkErrorsCommand));
    validate_memory_allocation("mark_errors_args_parser", data);

    assert_num_of_args(self, MARK_ERRORS_ARGS, MARK_ERRORS_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    parse_bool_arg(args[0], &data->setting);
    self->data.mark_errors = data;
}

/* Parses the arguments of a set command. */
void set_args_parser(Command *self, char **args, int num_of_args) {
    SetCommand *data = malloc(sizeof(SetCommand));
    validate_memory_allocation("set_args_parser", data);

    assert_num_of_args(self, SET_ARGS, SET_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    parse_int_arg(args[0], &data->column);
    parse_int_arg(args[1], &data->row);
    parse_int_arg(args[2], &data->value);
    self->data.set = data;
}

/* Parses the arguments of a guess command. */
void guess_args_parser(Command *self, char **args, int num_of_args) {
    GuessCommand *data = malloc(sizeof(GuessCommand));
    validate_memory_allocation("guess_args_parser", data);

    assert_num_of_args(self, GUESS_ARGS, GUESS_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    parse_double_arg(args[0], &data->threshold);
    self->data.guess = data;
}

/* Parses the arguments of a generate command. */
void generate_args_parser(Command *self, char **args, int num_of_args) {
    GenerateCommand *data = malloc(sizeof(GenerateCommand));
    validate_memory_allocation("generate_args_parser", data);

    assert_num_of_args(self, GENERATE_ARGS, GENERATE_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    parse_int_arg(args[0], &data->num_to_fill);
    parse_int_arg(args[1], &data->num_to_leave);
    self->data.generate = data;
}

/* Parses the arguments of a save command. */
void save_args_parser(Command *self, char **args, int num_of_args) {
    SaveCommand *data = malloc(sizeof(SaveCommand));
    validate_memory_allocation("save_args_parser", data);

    assert_num_of_args(self, SAVE_ARGS, SAVE_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    data->path = calloc(strlen(args[0]) + 1, sizeof(char));
    validate_memory_allocation("save_args_parser", data);
    strcpy(data->path, args[0]);
    self->data.save = data;
}

/* Parses the arguments of a hint command. */
void hint_args_parser(Command *self, char **args, int num_of_args) {
    HintCommand *data = malloc(sizeof(HintCommand));
    validate_memory_allocation("hint_args_parser", data);

    assert_num_of_args(self, HINT_ARGS, HINT_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    parse_int_arg(args[0], &data->column);
    parse_int_arg(args[1], &data->row);
    self->data.hint = data;
}

/* Parses the arguments of a guess_hint command. */
void guess_hint_args_parser(Command *self, char **args, int num_of_args) {
    GuessHintCommand *data = malloc(sizeof(GuessHintCommand));
    validate_memory_allocation("guess_hint_args_parser", data);

    assert_num_of_args(self, GUESS_HINT_ARGS, GUESS_HINT_ARGS, num_of_args);

    if (!is_valid(self)) {
        free(data);
        return;
    }

    parse_int_arg(args[0], &data->column);
    parse_int_arg(args[1], &data->row);
    self->data.guess_hint = data;
}

/* A basic parser for commands with no additional arguments. */
void basic_parser(Command *self, char **args, int num_of_args) {
    UNUSED(args);
    assert_num_of_args(self, NO_ARGS, NO_ARGS, num_of_args);
}


/* Configures the Command fields based on the parsed type. */
void configure_by_type(Command *command, char *type_str, GameMode mode) {
    command->_parse_args = basic_parser;

    if (strcmp(type_str, "solve") == 0) {
        command->type = solve;
        command->format = SOLVE_FORMAT;
        command->modes = SOLVE_MODES;
        command->_parse_args = solve_args_parser;
        command->_validate = solve_validator;
        command->_play = play_solve;

    } else if (strcmp(type_str, "edit") == 0) {
        command->type = edit;
        command->format = EDIT_FORMAT;
        command->modes = EDIT_MODES;
        command->_parse_args = edit_args_parser;
        command->_validate = edit_validator;
        command->_play = play_edit;

    } else if (strcmp(type_str, "mark_errors") == 0) {
        command->type = mark_errors;
        command->format = MARK_ERRORS_FORMAT;
        command->modes = MARK_ERRORS_MODES;
        command->_parse_args = mark_errors_args_parser;
        command->_validate = mark_errors_validator;
        command->_play = play_mark_errors;

    } else if (strcmp(type_str, "print_board") == 0) {
        command->type = print_board;
        command->format = PRINT_BOARD_FORMAT;
        command->modes = PRINT_BOARD_MODES;
        command->_play = play_print_board;

    } else if (strcmp(type_str, "set") == 0) {
        command->type = set;
        command->format = SET_FORMAT;
        command->modes = SET_MODES;
        command->_parse_args = set_args_parser;
        command->_validate = set_validator;
        command->_play = play_set;

    } else if (strcmp(type_str, "validate") == 0) {
        command->type = validate;
        command->format = VALIDATE_FORMAT;
        command->modes = VALIDATE_MODES;
        command->_validate = validate_validator;
        command->_play = play_validate;

    } else if (strcmp(type_str, "guess") == 0) {
        command->type = guess;
        command->format = GUESS_FORMAT;
        command->modes = GUESS_MODES;
        command->_parse_args = guess_args_parser;
        command->_validate = guess_validator;
        command->_play = play_guess;

    } else if (strcmp(type_str, "generate") == 0) {
        command->type = generate;
        command->format = GENERATE_FORMAT;
        command->modes = GENERATE_MODES;
        command->_parse_args = generate_args_parser;
        command->_validate = generate_validator;
        command->_play = play_generate;

    } else if (strcmp(type_str, "undo") == 0) {
        command->type = undo;
        command->format = UNDO_FORMAT;
        command->modes = UNDO_MODES;
        command->_validate = undo_validator;
        command->_play = play_undo;

    } else if (strcmp(type_str, "redo") == 0) {
        command->type = redo;
        command->format = REDO_FORMAT;
        command->modes = REDO_MODES;
        command->_validate = redo_validator;
        command->_play = play_redo;

    } else if (strcmp(type_str, "save") == 0) {
        command->type = save;
        command->format = SAVE_FORMAT;
        command->modes = SAVE_MODES;
        command->_parse_args = save_args_parser;
        command->_validate = save_validator;
        command->_play = play_save;

    } else if (strcmp(type_str, "hint") == 0) {
        command->type = hint;
        command->format = HINT_FORMAT;
        command->modes = HINT_MODES;
        command->_parse_args = hint_args_parser;
        command->_validate = hint_validator;
        command->_play = play_hint;

    } else if (strcmp(type_str, "guess_hint") == 0) {
        command->type = guess_hint;
        command->format = GUESS_HINT_FORMAT;
        command->modes = GUESS_HINT_MODES;
        command->_parse_args = guess_hint_args_parser;
        command->_validate = guess_hint_validator;
        command->_play = play_guess_hint;

    } else if (strcmp(type_str, "num_solutions") == 0) {
        command->type = num_solutions;
        command->format = NUM_SOLUTIONS_FORMAT;
        command->modes = NUM_SOLUTIONS_MODES;
        command->_validate = num_solutions_validator;
        command->_play = play_num_solutions;

    } else if (strcmp(type_str, "autofill") == 0) {
        command->type = autofill;
        command->format = AUTOFILL_FORMAT;
        command->modes = AUTOFILL_MODES;
        command->_validate = autofill_validator;
        command->_play = play_autofill;

    } else if (strcmp(type_str, "reset") == 0) {
        command->type = reset;
        command->format = RESET_FORMAT;
        command->modes = RESET_MODES;
        command->_play = play_reset;

    } else if (strcmp(type_str, "exit") == 0) {
        command->type = exit_game;
        command->format = EXIT_FORMAT;
        command->modes = EXIT_MODES;
        command->_play = play_exit_game;

    } else {
        if (mode == init_mode) {
            invalidate(command, INVALID_COMMAND_INIT_ERROR, invalid_command_name, false);
        } else if (mode == edit_mode) {
            invalidate(command, INVALID_COMMAND_EDIT_ERROR, invalid_command_name, false);
        } else if (mode == solve_mode) {
            invalidate(command, INVALID_COMMAND_SOLVE_ERROR, invalid_command_name, false);
        }
    }
}


/* main parsing function */

void parse_command(char *raw_command, Command *command, Game *game) {
    char *token;
    char* tokens[MAX_ARGS];
    int num_of_args = 0;


    /* get the first token (the type of the command, if exists) */
    token = strtok(raw_command, WHITESPACE);

    /* ignore empty lines (treated as 'empty' commands) */
    if (token == NULL) {
        command->type = empty;
        return;
    }

    configure_by_type(command, token, game->mode);

    /* walk through other tokens */
    token = strtok(NULL, WHITESPACE);
    while (token != NULL && num_of_args < MAX_ARGS) {
        tokens[num_of_args] = token;
        num_of_args++;
        token = strtok(NULL, WHITESPACE);
    }

    command->_parse_args(command, tokens, num_of_args);
}
