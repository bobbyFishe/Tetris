#include "tetris.h"

int run_tetris() {
  srand(time(NULL));
  WINDOW *scr = initscr();
  if (!scr) {
    fprintf(stderr, "Не удалось инициализировать ncurses\n");
    return 1;
  }
  keypad(stdscr, true);
  noecho();
  cbreak();
  nodelay(stdscr, true);
  curs_set(0);
  user_input(kStart, false);
  while (true) {
    int ch = getch();
    if (ch != ERR) {
      if (ch == 'q') {
        user_input(kTerminate, false);
      } else if (ch == 'p') {
        user_input(kPause, false);
      } else if (ch == KEY_LEFT) {
        user_input(kLeft, false);
      } else if (ch == KEY_RIGHT) {
        user_input(kRight, false);
      } else if (ch == KEY_DOWN) {
        user_input(kDown, false);
      } else if (ch == ' ') {
        user_input(kAction, false);
      }
    }

    GameInfo state = update_current_state();
    if (state.pause == -1) break;
    clear();
    print_field(state);
    napms(state.speed / 2);
  }
  endwin();
  return 0;
}

int main() { return run_tetris(); }