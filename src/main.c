#include <ncurses.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

enum field_size {
  ROW = 20,
  COLL = 10,
  ROW_COLL_FIGURE = 4,
  SPEED = 1000,
  FIGURE_POINTS = 4
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
} fsm_state;

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

typedef struct {
  int point_x;
  int point_y;
} Figure_point;

typedef struct {
  Figure_point figure_points[FIGURE_POINTS];
} Current_figure_points;

const int TETROMINOES[][ROW_COLL_FIGURE][ROW_COLL_FIGURE] = {
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
    {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}}
};

// Внутреннее состояние игры
typedef struct {
  fsm_state state;
  int figure_x;
  int figure_y;
  Current_figure_points current_figure;
  GameInfo_t game_info;
} GameState;

// Статическое состояние бэкенда
static GameState game_state = {0};

// Функции бэкенда
void free_matrix(int **matrix, int row);
int **alloc_matrix(int row, int col);
Current_figure_points figure_spawn(GameInfo_t *fsm_state, int figure_x, int figure_y);
void next_figure_generate(GameInfo_t *fsm_state);
int next_check(GameInfo_t *fsm_state);
void print_field(GameInfo_t fsm_state);
void start_fsm(GameInfo_t *game_fsm_state);
void spawn_fsm(GameInfo_t *game_fsm_state, Current_figure_points *current_figure, int *figure_x, int *figure_y, fsm_state *state);
void falling_fsm(GameInfo_t *game_fsm_state, Current_figure_points *current_figure, fsm_state *state);
void game_over_fsm(GameInfo_t *game_fsm_state);
void userInput(UserAction_t action, bool hold);
GameInfo_t updateCurrentState();

// Фронтенд
int main() {
  srand(time(NULL));
  initscr();
  keypad(stdscr, TRUE);
  noecho();
  cbreak();
  nodelay(stdscr, TRUE);
  curs_set(0);

  userInput(Start, false);
  while (1) {
    int ch = getch();
    if (ch == 'q') {
      userInput(Terminate, false);
    } else if (ch == 'p') {
      userInput(Pause, false);
    } else if (ch == KEY_LEFT) {
      userInput(Left, false);
    } else if (ch == KEY_RIGHT) {
      userInput(Right, false);
    } else if (ch == KEY_UP) {
      userInput(Up, false);
    } else if (ch == KEY_DOWN) {
      userInput(Down, false);
    } else if (ch == ' ') {
      userInput(Action, false);
    }

    GameInfo_t state = updateCurrentState();
    if (state.pause == -1) {
      break;
    }

    clear();
    print_field(state);
    napms(state.speed);
  }

  free_matrix(game_state.game_info.field, ROW);
  free_matrix(game_state.game_info.next, ROW_COLL_FIGURE);
  endwin();
  return 0;
}

// API бэкенда
void userInput(UserAction_t action, bool hold) {
  GameInfo_t *info = &game_state.game_info;

  switch (action) {
    case Start:
      if (game_state.state == START) {
        start_fsm(info);
        game_state.state = SPAWN;
      }
      break;
    case Pause:
      info->pause = !info->pause;
      if (!info->pause && game_state.state == PAUSED) {
        game_state.state = FALLING;
      }
      break;
    case Terminate:
      game_state.state = GAME_OVER;
      game_over_fsm(info);
      break;
    case Left:
    case Right:
      if (!info->pause && game_state.state == FALLING) {
        game_state.state = MOVING;
        // Реализация MOVING будет добавлена позже
        game_state.state = FALLING;
      }
      break;
    case Up:
      if (!info->pause && game_state.state == FALLING) {
        game_state.state = ROTATING;
        // Реализация ROTATING будет добавлена позже
        game_state.state = FALLING;
      }
      break;
    case Down:
    case Action:
      if (!info->pause && (game_state.state == FALLING || game_state.state == MOVING)) {
        game_state.state = FALLING;
        falling_fsm(info, &game_state.current_figure, &game_state.state);
      }
      break;
  }
}

GameInfo_t updateCurrentState() {
  GameInfo_t *info = &game_state.game_info;

  if (!info->pause && game_state.state != GAME_OVER) {
    switch (game_state.state) {
      case SPAWN:
        spawn_fsm(info, &game_state.current_figure, &game_state.figure_x, &game_state.figure_y, &game_state.state);
        break;
      case FALLING:
        falling_fsm(info, &game_state.current_figure, &game_state.state);
        break;
      case LOCKING:
        game_state.state = SPAWN;
        break;
      default:
        break;
    }
  }

  return *info;
}

// Вспомогательные функции
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

Current_figure_points figure_spawn(GameInfo_t *fsm_state, int figure_x, int figure_y) {
    Current_figure_points figure = {0};
    int count = 0;
    for (int i = 0; i < ROW_COLL_FIGURE && count < FIGURE_POINTS; i++) {
      for (int j = 0; j < ROW_COLL_FIGURE && count < FIGURE_POINTS; j++) {
        if (fsm_state->next[i][j]) {
          int new_x = j + figure_x;
          int new_y = i + figure_y;
          if (new_x >= 0 && new_x < COLL && new_y >= 0 && new_y < ROW) {
            fsm_state->field[new_y][new_x] = fsm_state->next[i][j];
            figure.figure_points[count].point_x = new_x;
            figure.figure_points[count].point_y = new_y;
            count++;
          }
        }
      }
    }
    // Проверка на корректное количество точек
    if (count != FIGURE_POINTS) {
        // Заполняем оставшиеся точки нулями для безопасности
        for (; count < FIGURE_POINTS; count++) {
        figure.figure_points[count].point_x = -1;
        figure.figure_points[count].point_y = -1;
        }
    }
    return figure;
  }

int next_check(GameInfo_t *fsm_state) {
  for (int i = 0; i < ROW_COLL_FIGURE; i++) {
    for (int j = 0; j < ROW_COLL_FIGURE; j++) {
      if (fsm_state->next[i][j]) {
        return 1;
      }
    }
  }
  return 0;
}

void print_field(GameInfo_t fsm_state) {
  for (int i = 0; i < ROW; i++) {
    for (int j = 0; j < COLL; j++) {
      mvprintw(i, j, fsm_state.field[i][j] ? "o" : " ");
    }
  }
  for (int i = 0; i < ROW; i++) {
    mvprintw(i, COLL, "|");
  }
  for (int i = 0; i < COLL + 1; i++) {
    mvprintw(ROW, i, "-");
  }
  mvprintw(0, COLL + 3, "Next");
  for (int i = 0; i < ROW_COLL_FIGURE; i++) {
    for (int j = 0; j < ROW_COLL_FIGURE; j++) {
      mvprintw(i + 2, j + COLL + 3, fsm_state.next[i][j] ? "o" : " ");
    }
  }
  mvprintw(6, COLL + 3, "Level: %d", fsm_state.level);
  mvprintw(7, COLL + 3, "Speed: %d", fsm_state.speed);
  mvprintw(8, COLL + 3, "Score: %d", fsm_state.score);
  refresh();
}

void start_fsm(GameInfo_t *game_fsm_state) {
  game_fsm_state->field = alloc_matrix(ROW, COLL);
  game_fsm_state->next = alloc_matrix(ROW_COLL_FIGURE, ROW_COLL_FIGURE);
  game_fsm_state->score = 0;
  game_fsm_state->high_score = 0;
  game_fsm_state->level = 1;
  game_fsm_state->speed = SPEED;
  game_fsm_state->pause = 0;
}

void spawn_fsm(GameInfo_t *game_fsm_state, Current_figure_points *current_figure, int *figure_x, int *figure_y, fsm_state *state) {
    
    *figure_x = COLL / 2 - 2; // figure_x = 3
    *figure_y = 0;
    if (!next_check(game_fsm_state)) {
      next_figure_generate(game_fsm_state);
    }

    int flag = 0;
    for (int i = 0; i < ROW_COLL_FIGURE && !flag; i++) {
        for (int j = 0; j < ROW_COLL_FIGURE && !flag; j++) {
        if (game_fsm_state->next[i][j]) {
            int new_x = j + *figure_x;
            int new_y = i + *figure_y;
            if (new_x >= 0 && new_x < COLL && new_y >= 0 && new_y < ROW) {
            if (game_fsm_state->field[new_y][new_x]) {
                *state = GAME_OVER;
                game_over_fsm(game_fsm_state);
                flag = 1;
            }
            }
        }
        }
    }
    if (!flag) {
      *current_figure = figure_spawn(game_fsm_state, *figure_x, *figure_y);
      next_figure_generate(game_fsm_state);
      *state = FALLING;
    }
  }

  void falling_fsm(GameInfo_t *game_fsm_state, Current_figure_points *current_figure, fsm_state *state) {
    int can_move = 1;
    // Найти нижние точки для каждой колонки
    int lowest_y[COLL] = {-1}; // Инициализируем -1 (нет точки)
    for (int i = 0; i < FIGURE_POINTS; i++) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < COLL && y >= 0 && y < ROW) {
        if (lowest_y[x] == -1 || y > lowest_y[x]) {
          lowest_y[x] = y;
        }
      }
    }
  
    // Проверяем только нижние точки
    for (int x = 0; x < COLL && can_move; x++) {
      if (lowest_y[x] != -1) {
        int new_y = lowest_y[x] + 1;
        if (new_y >= ROW) {
          can_move = 0;
        } else if (new_y >= 0 && new_y < ROW && game_fsm_state->field[new_y][x]) {
          can_move = 0;
        }
      }
    }
  
    if (can_move) {
      // Очищаем текущие позиции
      for (int i = 0; i < FIGURE_POINTS; i++) {
        int x = current_figure->figure_points[i].point_x;
        int y = current_figure->figure_points[i].point_y;
        if (x >= 0 && x < COLL && y >= 0 && y < ROW) {
          game_fsm_state->field[y][x] = 0;
        }
      }
      // Сдвигаем точки вниз
      for (int i = 0; i < FIGURE_POINTS; i++) {
        if (current_figure->figure_points[i].point_x >= 0) {
          current_figure->figure_points[i].point_y++;
        }
      }
      // Обновляем поле
      for (int i = 0; i < FIGURE_POINTS; i++) {
        int x = current_figure->figure_points[i].point_x;
        int y = current_figure->figure_points[i].point_y;
        if (x >= 0 && x < COLL && y >= 0 && y < ROW) {
          game_fsm_state->field[y][x] = 1;
        }
      }
    } else {
      *state = LOCKING;
    }
  }

void game_over_fsm(GameInfo_t *game_fsm_state) {
  game_fsm_state->pause = -1;
}

void next_figure_generate(GameInfo_t *fsm_state) {
  int random_index = rand() % (sizeof(TETROMINOES) / sizeof(TETROMINOES[0]));
  for (int i = 0; i < ROW_COLL_FIGURE; i++) {
    for (int j = 0; j < ROW_COLL_FIGURE; j++) {
      fsm_state->next[i][j] = TETROMINOES[random_index][i][j];
    }
  }
}