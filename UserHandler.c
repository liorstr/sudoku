#include <stdio.h>
#include <stdlib.h>
#include "UserHandler.h"
#include "io/Command.h"
#include "io/Parser.h"
#include "io/validators.h"


#define MAX_COMMAND_LEN 256
#define INPUT_LEN (MAX_COMMAND_LEN + 2)

#define COMMAND_TOO_LONG_ERROR "Error: command is too long\n"

#define HEADLINE \
"         _____           _       _           ___  ___          _                     \n" \
"        /  ___|         | |     | |          |  \\/  |         | |                    \n" \
"        \\ `--. _   _  __| | ___ | | ___   _  | .  . | __ _  __| |_ __   ___  ___ ___ \n" \
"         `--. \\ | | |/ _` |/ _ \\| |/ / | | | | |\\/| |/ _` |/ _` | '_ \\ / _ \\/ __/ __|\n" \
"        /\\__/ / |_| | (_| | (_) |   <| |_| | | |  | | (_| | (_| | | | |  __/\\__ \\__ \\\n" \
"        \\____/ \\__,_|\\__,_|\\___/|_|\\_\\\\__,_| \\_|  |_/\\__,_|\\__,_|_| |_|\\___||___/___/\n" \
"                                                                                     \n\n" \


void handle_eof(Game *game, Command *command) {
    if (feof(stdin)) {
        announce_exit();
        destroy_command(command);
        destroy_game(game);
        exit(0);
    }
}

void get_user_command(Game *game, Command *command) {
    char raw_command[INPUT_LEN] = {0};

    printf("Please enter a command:\n");
    fgets(raw_command, INPUT_LEN, stdin);
    handle_eof(game, command);
    if (raw_command[MAX_COMMAND_LEN] != 0) {
        invalidate(command, COMMAND_TOO_LONG_ERROR, invalid_command_length);
        fflush(stdin);
        return;
    }

    parse_command(raw_command,  command, game);
    if (command->type == empty) {
        return;
    }

    validate_command(command, game);
}


void announce_game_start() {
    printf("%s Welcome! Let the Games Begin!\n\n", HEADLINE);
}

void announce_game_won() {
    printf("*~*~*~*~* Woo Hoo! You did it! *~*~*~*~* \nPlease load another game or exit.\n\n");
}

void announce_game_erroneous() {
    printf("Hmm this isn't quite right. You have some errors you need to fix in order to complete the game.\n\n");
}

void announce_changes_made() {
    printf("The following changes were made: \n-------------------------------\n");
}

void print_change(Change *change, bool reverted) {
    int from_value, to_value;

    if (reverted) {
        from_value = change->value;
        to_value = change->prev_value;
    } else {
        to_value = change->value;
        from_value = change->prev_value;
    }
    printf(" @ Cell (%d, %d): [%d] --> [%d]\n", change->column, change->row, from_value, to_value);
}

void announce_error(Error *error) {
    printf("%s\n", error->message);
}

void announce_exit() {
    printf("Exiting...\n");
}