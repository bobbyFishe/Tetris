#include "tetris.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Number of rotations per tetromino type (I, L, O, T, S, Z, J).
static const int kRotationsPerTetromino[] = {2, 4, 1, 4, 2, 2, 4};
static GameState game_state = {0};

// Debug: Печать занятых клеток поля
void debug_field(GameInfo *game_info, int start_y, int end_y) {
  int line = 18;
  for (int i = start_y; i <= end_y && i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      if (game_info->field[i][j]) {
        mvprintw(line++, kCol + 2, "Field[%d][%d]=%d", i, j, game_info->field[i][j]);
        FILE *log = fopen("tetris.log", "a");
        fprintf(log, "Field[%d][%d]=%d\n", i, j, game_info->field[i][j]);
        fclose(log);
      }
    }
  }
}

// Allocates a matrix of given dimensions.
int **alloc_matrix(int rows, int cols) {
  int **matrix = malloc(rows * sizeof(int *));
  if (!matrix) return NULL;
  for (int i = 0; i < rows; ++i) {
    matrix[i] = calloc(cols, sizeof(int));
    if (!matrix[i]) {
      for (int j = 0; j < i; ++j) free(matrix[j]);
      free(matrix);
      return NULL;
    }
  }
  return matrix;
}

// Frees a matrix.
void free_matrix(int **matrix, int rows) {
  if (matrix) {
    for (int i = 0; i < rows; ++i) {
      free(matrix[i]);
    }
    free(matrix);
  }
}

// Spawns a new tetromino on the field.
CurrentFigurePoints figure_spawn(GameInfo *game_info, int figure_x, int figure_y,
                                int figure_type, int rotation_idx) {
  CurrentFigurePoints figure = {0};
  int count = 0;
  const int (*tetromino)[kFigureSize] = kTetrominoes[figure_type][rotation_idx];
  for (int i = 0; i < kFigureSize && count < kFigurePoints; ++i) {
    for (int j = 0; j < kFigureSize && count < kFigurePoints; ++j) {
      if (tetromino[i][j]) {
        int new_x = j + figure_x;
        int new_y = i + figure_y;
        if (new_x >= 0 && new_x < kCol && new_y >= 0 && new_y < kRow) {
          game_info->field[new_y][new_x] = tetromino[i][j];
          figure.figure_points[count].point_x = new_x;
          figure.figure_points[count].point_y = new_y;
          count++;
        }
      }
    }
  }
  if (count != kFigurePoints) {
    //mvprintw(16, kCol + 2, "Spawn Error: Only %d points", count);
    for (; count < kFigurePoints; ++count) {
      figure.figure_points[count].point_x = -1;
      figure.figure_points[count].point_y = -1;
    }
  }
  //debug_field(game_info, 0, 5);
  return figure;
}

// Generates a new tetromino for the next slot.
void next_figure_generate(GameInfo *game_info, int *figure_type, int *rotation_idx) {
  *figure_type = rand() % (sizeof(kTetrominoes) / sizeof(kTetrominoes[0]));
  *rotation_idx = 0;
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      game_info->next[i][j] = kTetrominoes[*figure_type][*rotation_idx][i][j];
    }
  }
}

// Checks if the next tetromino exists.
int next_check(const GameInfo *game_info) {
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      if (game_info->next[i][j]) return 1;
    }
  }
  return 0;
}

// Renders the game field and UI.
void print_field(const GameInfo game_info) {
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      mvprintw(i, j, game_info.field[i][j] ? "o" : " ");
    }
  }
  for (int i = 0; i < kRow; ++i) {
    mvprintw(i, kCol, "|");
  }
  for (int i = 0; i < kCol + 1; ++i) {
    mvprintw(kRow, i, "-");
  }
  mvprintw(0, kCol + 3, "Next");
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      mvprintw(i + 2, j + kCol + 3, game_info.next[i][j] ? "o" : " ");
    }
  }
  mvprintw(6, kCol + 3, "Level: %d", game_info.level);
  mvprintw(7, kCol + 3, "Speed: %d", game_info.speed);
  mvprintw(8, kCol + 3, "Score: %d", game_info.score);
  refresh();
}

// Initializes the game state.
void start_fsm(GameInfo *game_info) {
  game_info->field = alloc_matrix(kRow, kCol);
  game_info->next = alloc_matrix(kFigureSize, kFigureSize);
  game_info->score = 0;
  game_info->high_score = 0;
  game_info->level = 1;
  game_info->speed = kSpeed;
  game_info->pause = 0;

  // // Debug: Проверяем инициализацию
  // bool field_empty = true;
  // for (int i = 0; i < kRow && field_empty; ++i) {
  //   for (int j = 0; j < kCol; ++j) {
  //     if (game_info->field[i][j]) {
  //       mvprintw(18, kCol + 2, "Field init error: [%d][%d]=%d", i, j, game_info->field[i][j]);
  //       field_empty = false;
  //       break;
  //     }
  //   }
  // }
  // if (field_empty) {
  //   mvprintw(18, kCol + 2, "Field initialized empty");
  // }
  // refresh();
}

// Handles the spawning of a new tetromino.
void spawn_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
               int *figure_x, int *figure_y, FsmState *state) {
  *figure_x = kCol / 2 - kFigureSize / 2;
  *figure_y = 0;

  if (!next_check(game_info)) {
    next_figure_generate(game_info, &game_state.next_figure_type, &game_state.rotation_idx);
  }

  game_state.figure_type = game_state.next_figure_type;
  game_state.rotation_idx = 0;

  bool collision = false;
  const int (*tetromino)[kFigureSize] = kTetrominoes[game_state.figure_type][game_state.rotation_idx];
  for (int i = 0; i < kFigureSize && !collision; ++i) {
    for (int j = 0; j < kFigureSize && !collision; ++j) {
      if (tetromino[i][j]) {
        int new_x = j + *figure_x;
        int new_y = i + *figure_y;
        if (new_x >= 0 && new_x < kCol && new_y >= 0 && new_y < kRow) {
          if (game_info->field[new_y][new_x]) {
            *state = GAME_OVER;
            game_over_fsm(game_info);
            collision = true;
            // mvprintw(15, kCol + 2, "Spawn Collision at (%d,%d)", new_x, new_y);
            // refresh();
          }
        }
      }
    }
  }
  if (!collision) {
    *current_figure = figure_spawn(game_info, *figure_x, *figure_y,
                                  game_state.figure_type, game_state.rotation_idx);
    next_figure_generate(game_info, &game_state.next_figure_type, &game_state.rotation_idx);
    *state = FALLING;
  }

  // mvprintw(10, kCol + 2, "Spawn: Type=%d, Next=%d, Rot=%d, Pos=(%d,%d)",
  //          game_state.figure_type, game_state.next_figure_type, game_state.rotation_idx,
  //          *figure_x, *figure_y);
  // for (int i = 0; i < kFigurePoints; ++i) {
  //   mvprintw(11 + i, kCol + 2, "Point %d: (%d,%d)", i,
  //            current_figure->figure_points[i].point_x,
  //            current_figure->figure_points[i].point_y);
  // }
  // refresh();
}

// Rotates the current tetromino clockwise.
bool rotate_figure(GameInfo *game_info, CurrentFigurePoints *current_figure) {
  int figure_type = game_state.figure_type;
  int current_rotation = game_state.rotation_idx;
  int num_rotations = kRotationsPerTetromino[figure_type];

  if (num_rotations == 1) {
    //mvprintw(0, kCol + 2, "O-shape: No rotation");
    //refresh();
    return true;
  }

  int next_rotation = (current_rotation + 1) % num_rotations;
  const int (*tetromino)[kFigureSize] = kTetrominoes[figure_type][next_rotation];
  int offsets[7][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {2, 0}, {-2, 0}};
  int num_offsets = (figure_type == 0) ? 7 : 5;

  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      game_info->field[y][x] = 0;
    }
  }

  // mvprintw(0, kCol + 2, "Try rotate: Type=%d, Rot=%d", figure_type, next_rotation);
  // mvprintw(1, kCol + 2, "Pos: (%d, %d)", game_state.figure_x, game_state.figure_y);
  // for (int i = 0; i < kFigurePoints; ++i) {
  //   mvprintw(2 + i, kCol + 2, "Point %d: (%d, %d)", i,
  //            current_figure->figure_points[i].point_x,
  //            current_figure->figure_points[i].point_y);
  // }

  for (int k = 0; k < num_offsets; ++k) {
    int offset_x = offsets[k][0];
    int offset_y = offsets[k][1];
    int new_figure_x = game_state.figure_x + offset_x;
    int new_figure_y = game_state.figure_y + offset_y;

    bool valid = true;
    //int fail_x = -1, fail_y = -1;
    //bool is_collision = false;
    for (int i = 0; i < kFigureSize && valid; ++i) {
      for (int j = 0; j < kFigureSize && valid; ++j) {
        if (tetromino[i][j]) {
          int new_x = j + new_figure_x;
          int new_y = i + new_figure_y;
          if (new_x < 0 || new_x >= kCol || new_y < 0 || new_y >= kRow) {
            valid = false;
            // fail_x = new_x;
            // fail_y = new_y;
          } else if (game_info->field[new_y][new_x]) {
            valid = false;
            // fail_x = new_x;
            // fail_y = new_y;
            //is_collision = true;
          }
        }
      }
    }

    if (valid) {
      game_state.rotation_idx = next_rotation;
      game_state.figure_x = new_figure_x;
      game_state.figure_y = new_figure_y;
      *current_figure = figure_spawn(game_info, new_figure_x, new_figure_y,
                                    figure_type, next_rotation);
      // mvprintw(0, kCol + 2, "Rotated: Type=%d, Rot=%d", figure_type, next_rotation);
      // mvprintw(1, kCol + 2, "Pos: (%d, %d), Offset: (%d, %d)", new_figure_x, new_figure_y, offset_x, offset_y);
      // for (int i = 0; i < kFigurePoints; ++i) {
      //   mvprintw(2 + i, kCol + 2, "Point %d: (%d, %d)", i,
      //            current_figure->figure_points[i].point_x,
      //            current_figure->figure_points[i].point_y);
      // }
      // refresh();
      return true;
    }

    // mvprintw(6 + k, kCol + 2, "Offset (%d, %d): %s at (%d, %d)",
    //          offset_x, offset_y, is_collision ? "Collision" : "Out of bounds", fail_x, fail_y);
  }

  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      game_info->field[y][x] = 1;
    }
  }

  // mvprintw(0, kCol + 2, "Rotation failed: Type=%d, Rot=%d", figure_type, next_rotation);
  // mvprintw(1, kCol + 2, "Pos: (%d, %d)", game_state.figure_x, game_state.figure_y);
  // refresh();
  return false;
}

// Handles the falling of the current tetromino.
void falling_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
                 FsmState *state) {
  bool can_move = true;
  int lowest_y[kCol];
  for (int i = 0; i < kCol; ++i) lowest_y[i] = -1;
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      if (lowest_y[x] == -1 || y > lowest_y[x]) {
        lowest_y[x] = y;
      }
    }
  }
  for (int x = 0; x < kCol && can_move; ++x) {
    if (lowest_y[x] != -1) {
      int new_y = lowest_y[x] + 1;
      if (new_y >= kRow) {
        can_move = false;
      } else if (new_y >= 0 && new_y < kRow && game_info->field[new_y][x]) {
        can_move = false;
      }
    }
  }

  if (can_move) {
    // Очистка старых позиций
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 0;
      }
    }
    // Обновление координат
    for (int i = 0; i < kFigurePoints; ++i) {
      if (current_figure->figure_points[i].point_x >= 0) {
        current_figure->figure_points[i].point_y++;
      }
    }
    // Синхронизация figure_y с минимальным y точек
    int min_y = kRow;
    for (int i = 0; i < kFigurePoints; ++i) {
      if (current_figure->figure_points[i].point_y >= 0 && current_figure->figure_points[i].point_y < min_y) {
        min_y = current_figure->figure_points[i].point_y;
      }
    }
    game_state.figure_y = min_y;
    // Перерисовка
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 1;
      }
    }
    // Debug: Поле после шага
    // mvprintw(17, kCol + 2, "Falling: Pos=(%d,%d)", game_state.figure_x, game_state.figure_y);
    // debug_field(game_info, 0, 10);
  } else {
    *state = LOCKING;
  }
}

// Handles the moving of the current tetromino left or right.
void moving_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
                FsmState *state, int *figure_x, UserAction direction) {
  bool can_move = true;
  //int fail_x = -1, fail_y = -1;
  int delta_x = (direction == kLeft) ? -1 : 1;

  // mvprintw(12, kCol + 2, "Move %s: figure_x=%d, figure_y=%d", direction == kLeft ? "Left" : "Right", *figure_x, game_state.figure_y);
  // for (int i = 0; i < kFigurePoints; ++i) {
  //   mvprintw(13 + i, kCol + 2, "Point %d: (%d,%d)", i,
  //             current_figure->figure_points[i].point_x,
  //             current_figure->figure_points[i].point_y);
  // }
  // debug_field(game_info, 0, 10);

  // Массивы для хранения минимальных/максимальных x для каждой y
  int extreme_x[kRow];
  int points_per_y[kRow]; // Количество точек для каждой y
  for (int i = 0; i < kRow; ++i) {
    extreme_x[i] = (direction == kLeft) ? kCol : -1; // Инициализация: max для kLeft, min для kRight
    points_per_y[i] = 0;
  }

  // Находим крайние точки
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (y >= 0 && y < kRow) {
      points_per_y[y]++;
      if (direction == kLeft) {
        if (x < extreme_x[y]) extreme_x[y] = x; // Минимальный x для kLeft
      } else {
        if (x > extreme_x[y]) extreme_x[y] = x; // Максимальный x для kRight
      }
    }
  }

  // Проверяем только крайние точки
  for (int y = 0; y < kRow && can_move; ++y) {
    if (points_per_y[y] > 0) { // Проверяем только строки с точками
      int x = extreme_x[y] + delta_x;
      if (y < 0 || y >= kRow) {
        can_move = false;
        // fail_x = x;
        // fail_y = y;
        //mvprintw(17, kCol + 2, "Invalid y=%d", y);
      } else if (x < 0 || x >= kCol) {
        can_move = false;
        // fail_x = x;
        // fail_y = y;
      } else if (game_info->field[y][x]) {
        can_move = false;
        // fail_x = x;
        // fail_y = y;
      }
    }
  }

  if (can_move) {
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 0;
      }
    }
    for (int i = 0; i < kFigurePoints; ++i) {
      if (current_figure->figure_points[i].point_x >= 0) {
        current_figure->figure_points[i].point_x += delta_x;
      }
    }
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 1;
      }
    }
    *figure_x += delta_x;
    falling_fsm(game_info, current_figure, state);
    // mvprintw(11, kCol + 2, "Move %s: Pos=(%d,%d)",
    //          direction == kLeft ? "Left" : "Right", *figure_x, game_state.figure_y);
  } 
  // else {
  //   mvprintw(11, kCol + 2, "Move %s Failed: %s at (%d,%d)",
  //            direction == kLeft ? "Left" : "Right",
  //            (fail_x < 0 || fail_x >= kCol) ? "Out of bounds" :
  //            (fail_y < 0 || fail_y >= kRow) ? "Invalid y" : "Collision",
  //            fail_x, fail_y);
  //   if (!(fail_x < 0 || fail_x >= kCol) && !(fail_y < 0 || fail_y >= kRow)) {
  //     mvprintw(17, kCol + 2, "Field[%d][%d]=%d", fail_y, fail_x, game_info->field[fail_y][fail_x]);
  //   }
  // }
  // refresh();
  *state = FALLING;
}

// Sets the game to the game-over state.
void game_over_fsm(GameInfo *game_info) {
  game_info->pause = -1;
}

// Processes user input to control the game.
void user_input(UserAction action, bool hold) {
  GameInfo *info = &game_state.game_info;
  switch (action) {
    case kStart:
      if (game_state.state == START) {
        start_fsm(info);
        game_state.state = SPAWN;
      }
      break;
    case kPause:
      info->pause = !info->pause;
      if (!info->pause && game_state.state == PAUSED) {
        game_state.state = FALLING;
      }
      break;
    case kTerminate:
      game_state.state = GAME_OVER;
      game_over_fsm(info);
      break;
    case kLeft:
    case kRight:
      if (!info->pause && game_state.state == FALLING) {
        game_state.move_direction = action;
        game_state.state = MOVING;

      }
      break;
    case kUp:
      break;
    case kDown:
      if (!info->pause && (game_state.state == FALLING || game_state.state == MOVING)) {
        game_state.state = FALLING;
        if (hold) {
          falling_fsm(info, &game_state.current_figure, &game_state.state);
        }
        while (game_state.state == FALLING) {
          falling_fsm(info, &game_state.current_figure, &game_state.state);
        }
      }
      break;
    case kAction:
      if (!info->pause && game_state.state == FALLING) {
        game_state.state = ROTATING;
        rotate_figure(info, &game_state.current_figure);
        game_state.state = FALLING;
      }
      break;
  }
}

// Updates the game state and returns the current state for rendering.
GameInfo update_current_state() {
  GameInfo *info = &game_state.game_info;
  if (!info->pause && game_state.state != GAME_OVER) {
    switch (game_state.state) {
      case SPAWN:
        spawn_fsm(info, &game_state.current_figure, &game_state.figure_x,
                  &game_state.figure_y, &game_state.state);
        break;
      case FALLING:
        falling_fsm(info, &game_state.current_figure, &game_state.state);
        break;
      case MOVING:
        moving_fsm(info, &game_state.current_figure, &game_state.state,
                   &game_state.figure_x, game_state.move_direction);
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

// Initializes and runs the Tetris game.
int run_tetris() {
  srand(time(NULL));
  initscr();
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

  free_matrix(game_state.game_info.field, kRow);
  free_matrix(game_state.game_info.next, kFigureSize);
  endwin();
  return 0;
}

int main() {
  return run_tetris();
}