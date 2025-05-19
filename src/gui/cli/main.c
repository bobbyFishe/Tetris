#include "tetris.h"

/**
 * Initializes and runs the Tetris game with ncurses.
 * @return 0 on successful termination, non-zero on error.
 */
int runTetris() {
  srand(time(NULL));
  WINDOW* scr = initscr();
  if (!scr) {
    fprintf(stderr, "Failed to initialize ncurses\n");
    return 1;
  }
  keypad(stdscr, true);
  noecho();
  cbreak();
  nodelay(stdscr, true);
  curs_set(0);
  userInput(kActionStart, false);
  while (true) {
    int ch = getch();
    if (ch != ERR) {
      switch (ch) {
        case 'q':
          userInput(kActionTerminate, false);
          break;
        case 'p':
          userInput(kActionPause, false);
          break;
        case KEY_LEFT:
          userInput(kActionLeft, false);
          break;
        case KEY_RIGHT:
          userInput(kActionRight, false);
          break;
        case KEY_DOWN:
          userInput(kActionDown, false);
          break;
        case ' ':
          userInput(kActionRotate, false);
          break;
      }
    }
    GameInfo state = updateCurrentState();
    if (state.pause == -1) break;
    clear();
    renderField(state);
    napms(state.speed);
  }
  endwin();
  return 0;
}

/**
 * Entry point for the Tetris game.
 * @return 0 on successful termination, non-zero on error.
 */
int main() { return runTetris(); }