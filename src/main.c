#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum field_size {
  ROW = 20,
  COLL = 10,
  ROW_COLL_FIGURE = 4,
  SPEED = 1000,
  POINTS_COUNT = 0,
};

typedef enum {
  START,
  SPAWN,
  FALLING,
  MOVING,
  ROTATING,
  LOCKING,
  CLEARING,
  PAUSED,
  GAME_OVER
} State;

typedef enum {
  Start,
  Pause,
  Terminate,
  Left,
  Right,
  Up,
  Down,
  Action
} UserAction_t;

typedef struct {
  int **field;
  int **next;
  int score;
  int high_score;
  int level;
  int speed;
  int pause;
} GameInfo_t;

const int TETROMINOES[][ROW_COLL_FIGURE][ROW_COLL_FIGURE] = {
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
    {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}};

void free_matrix(int **matrix, int row);
int **alloc_matrix(int row, int col);
void figure_spawn(GameInfo_t *state);
void next_figure_generate(GameInfo_t *state);
GameInfo_t updateCurrentState();
void print_field(GameInfo_t state);
void start_fsm(GameInfo_t *game_state);
void spawn_fsm(GameInfo_t *game_state);
void faling_fsm(GameInfo_t *game_state);

// void userInput(UserAction_t action, bool hold);
State proccess_input(GameInfo_t *game_state, State fsm);

int main() {
  GameInfo_t current_state;
  initscr();             // Переход в curses-режим
  keypad(stdscr, TRUE);  // Включает клавиатуру
  noecho();  // Отключает вывод на экран нажатие клавиш
  cbreak();  // Разрешает непременный режим работы с терминалом
  nodelay(stdscr, TRUE);  // Не блокирует getch()
  curs_set(0);
  while (1) {
    clear();
    current_state = updateCurrentState();
    print_field(current_state);
    char ch = getch();
    if (ch == 'q') {
      break;
    }
    napms(SPEED);
  }
  free_matrix(current_state.field, ROW);
  free_matrix(current_state.next, ROW_COLL_FIGURE);
  endwin();
  return 0;
}

void free_matrix(int **matrix, int row) {
  if (matrix) {
    for (int i = 0; i < row; i++) {
      free(matrix[i]);
    }
    free(matrix);
  }
}

int **alloc_matrix(int row, int col) {
  int **matrix = (int **)malloc(row * sizeof(int *));
  for (int i = 0; i < row; i++) {
    matrix[i] = (int *)calloc(col, sizeof(int));
  }
  return matrix;
}

void figure_spawn(GameInfo_t *state) {
  int x = COLL / 2 - 2;

  for (int i = 0; i < ROW_COLL_FIGURE; i++) {
    for (int j = 0; j < ROW_COLL_FIGURE; j++) {
      state->field[i][x + j] = state->next[i][j];
    }
  }
}

// int game_state_start_check(GameInfo_t *state) {
//   int flag = 0;
//   if (state->field != NULL || state->next != NULL || state->field != 0 ||
//       state->high_score != 0 || state->level != 0 || state->pause != 0 ||
//       state->speed != 0)
//     flag = 1;
//   return flag;
// }

GameInfo_t updateCurrentState() {
  static GameInfo_t game_state = {0};
  static State fsm = START;
  fsm = proccess_input(&game_state, fsm);

  return game_state;
}

void print_field(GameInfo_t state) {
  for (int i = 0; i < ROW; i++) {
    for (int j = 0; j < COLL; j++) {
      mvprintw(i, j, state.field[i][j] ? "o" : " ");
    }
  }
  for (int i = 0; i < ROW; i++) {
    mvprintw(i, COLL, "|");
  }
  for (int i = 0; i < COLL + 1; i++) {
    mvprintw(ROW, i, "-");
  }

  // Вывод информации
  mvprintw(0, COLL + 3, "Next");
  for (int i = 0; i < ROW_COLL_FIGURE; i++) {
    for (int j = 0; j < ROW_COLL_FIGURE; j++) {
      mvprintw(i + 2, j + COLL + 3, state.next[i][j] ? "o" : " ");
    }
  }
  mvprintw(6, COLL + 3, "Level: %d", state.level);
  mvprintw(7, COLL + 3, "Speed: %d", state.speed);
  mvprintw(8, COLL + 3, "Score: %d", state.score);
  refresh();
}

void start_fsm(GameInfo_t *game_state) {
  game_state->field = alloc_matrix(ROW, COLL);
  game_state->next = alloc_matrix(ROW_COLL_FIGURE, ROW_COLL_FIGURE);
  game_state->score = 0;
  game_state->high_score = 0;
  game_state->level = 1;
  game_state->speed = 1;
  game_state->pause = 0;
}

void spawn_fsm(GameInfo_t *game_state) {
  next_figure_generate(game_state);
  figure_spawn(game_state);
  next_figure_generate(game_state);
}

void faling_fsm(GameInfo_t *game_state) {
  
}

State proccess_input(GameInfo_t *game_state, State fsm) {
  switch (fsm) {
    case START:
      start_fsm(game_state);
      fsm = SPAWN;
      break;
    case SPAWN:
      spawn_fsm(game_state);
      fsm = FALLING;
      ///
      break;
    case FALLING:
      faling_fsm(game_state);
      ///
      break;
    case MOVING:
      /* code */
      break;
    case ROTATING:
      /* code */
      break;
    case LOCKING:
      /* code */
      break;
    case CLEARING:
      /* code */
      break;
    case PAUSED:
      ////
      break;
    case GAME_OVER:
      ////
      break;
  }
  return fsm;
}

void next_figure_generate(GameInfo_t *state) {
  int random_index = rand() % (sizeof(TETROMINOES) / sizeof(TETROMINOES[0]));
  for (int i = 0; i < ROW_COLL_FIGURE; i++) {
    for (int j = 0; j < ROW_COLL_FIGURE; j++) {
      state->next[i][j] = TETROMINOES[random_index][i][j];
    }
  }
}

// int next_checking(int **next) {
//   int flag = 0;
//   for (int i = 0; i < ROW_COLL_FIGURE && !flag; i++) {
//     for (int j = 0; j < ROW_COLL_FIGURE && !flag; j++) {
//       if (next[i][j]) flag = 1;
//     }
//   }
//   return flag;
// }

// int next_row_checking(int **next) {
//   int flag = 0;
//   int i = 0;
//   for (i = 0; i < ROW_COLL_FIGURE && !flag; i++) {
//     for (int j = 0; j < ROW_COLL_FIGURE && !flag; j++) {
//       if (next[i][j]) flag = 1;
//     }
//   }
//   return i;
// }