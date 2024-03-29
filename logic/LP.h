#ifndef FINAL_PROJECT_LP_H
#define FINAL_PROJECT_LP_H

#include "../components/Board.h"
#include "../components/StatesList.h"

/**
 * Guesses a solution for the given board using LP (not ILP!), and fills the
 * board so that only guesses over the threshold are considered.
 *
 * Note: if more than one possible value is found, randomly chooses a value,
 *       using the scores as weights for the random choice.
 * @param board: the board to guess a solution for.
 * @param states: the states list to update with the guessed solution.
 * @param threshold: the threshold for the guess.
 * @return: true in case of success, and false in case of failure.
 */
bool guess_solution(Board *board, States *states, double threshold);

/**
 * Returns an array of scores that represents the possible values and their
 * score for the given cell (the size of the array is always the board dimension).
 * For example, the array [0, 0.1, 0.33, 0, 0.2, ...] means that the value 2 has a
 * score of 0.1, and the value 3 has a score of 0.33, and so on.
 * @param board: the board to guess a solution for.
 * @param row: the index of the cell's row (zero-based).
 * @param column: the index of the cell's column (zero-based).
 * @return: array of scores for the possible values.
 */
double* get_cell_guesses(Board *board, int row, int column);


#endif