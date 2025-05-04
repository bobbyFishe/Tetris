#include "tetris.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Get number of rotations per tetromino type (I, L, O, T, S, Z, J).
const int *get_rotations_per_tetromino() {
  static const int rotations[] = {2, 4, 1, 4, 2, 2, 4};
  return rotations;
}

// Get the game state singleton.
GameState *get_game_state() {
  static GameState game_state = {0};
  return &game_state;
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
CurrentFigurePoints figure_spawn(GameInfo *game_info, int figure_x,
                                 int figure_y, int figure_type,
                                 int rotation_idx) {
  CurrentFigurePoints figure = {0};
  int count = 0;
  const int(*tetromino)[kFigureSize] = kTetrominoes[figure_type][rotation_idx];
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
    for (; count < kFigurePoints; ++count) {
      figure.figure_points[count].point_x = -1;
      figure.figure_points[count].point_y = -1;
    }
  }
  return figure;
}

// Generates a new tetromino for the next slot.
void next_figure_generate(GameInfo *game_info, int *figure_type,
                          int *rotation_idx) {
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
  mvprintw(7, kCol + 3, "Score: %d", game_info.score);
  mvprintw(8, kCol + 3, "High Score: %d", game_info.high_score);
  if (game_info.pause) {
    mvprintw(kRow / 2, kCol / 2 - 3, "PAUSED");
  }
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
  get_game_state()->points_toward_level = 0;

  FILE *file = fopen("high_score.txt", "r");
  if (file) {
    if (fscanf(file, "%d", &game_info->high_score) != 1) {
      game_info->high_score = 0;
    }
    fclose(file);
  }
}

// Handles the spawning of a new tetromino.
void spawn_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
               int *figure_x, int *figure_y, FsmState *state) {
  GameState *gs = get_game_state();
  *figure_x = kCol / 2 - kFigureSize / 2;
  *figure_y = 0;

  if (!next_check(game_info)) {
    next_figure_generate(game_info, &gs->next_figure_type, &gs->rotation_idx);
  }

  gs->figure_type = gs->next_figure_type;
  gs->rotation_idx = 0;

  bool collision = false;
  const int(*tetromino)[kFigureSize] =
      kTetrominoes[gs->figure_type][gs->rotation_idx];
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
          }
        }
      }
    }
  }
  if (!collision) {
    *current_figure = figure_spawn(game_info, *figure_x, *figure_y,
                                   gs->figure_type, gs->rotation_idx);
    next_figure_generate(game_info, &gs->next_figure_type, &gs->rotation_idx);
    *state = FALLING;
  }
}

// Rotates the current tetromino clockwise.
void rotate_figure(GameInfo *game_info, CurrentFigurePoints *current_figure) {
  GameState *gs = get_game_state();
  const int *rotations = get_rotations_per_tetromino();
  int figure_type = gs->figure_type;
  int current_rotation = gs->rotation_idx;
  int num_rotations = rotations[figure_type];

  if (num_rotations > 1) {
    int next_rotation = (current_rotation + 1) % num_rotations;
    const int(*tetromino)[kFigureSize] =
        kTetrominoes[figure_type][next_rotation];
    int offsets[7][2] = {{0, 0},  {1, 0}, {-1, 0}, {0, 1},
                         {0, -1}, {2, 0}, {-2, 0}};
    int num_offsets = (figure_type == 0) ? 7 : 5;

    // Очистка текущей фигуры
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 0;
      }
    }

    bool rotated = false;
    for (int k = 0; k < num_offsets; ++k) {
      int offset_x = offsets[k][0];
      int offset_y = offsets[k][1];
      int new_figure_x = gs->figure_x + offset_x;
      int new_figure_y = gs->figure_y + offset_y;

      bool valid = true;
      for (int i = 0; i < kFigureSize && valid; ++i) {
        for (int j = 0; j < kFigureSize && valid; ++j) {
          if (tetromino[i][j]) {
            int new_x = j + new_figure_x;
            int new_y = i + new_figure_y;
            if (new_x < 0 || new_x >= kCol || new_y < 0 || new_y >= kRow) {
              valid = false;
            } else if (game_info->field[new_y][new_x]) {
              valid = false;
            }
          }
        }
      }

      if (valid) {
        gs->rotation_idx = next_rotation;
        gs->figure_x = new_figure_x;
        gs->figure_y = new_figure_y;
        *current_figure = figure_spawn(game_info, new_figure_x, new_figure_y,
                                       figure_type, next_rotation);
        rotated = true;
        break;
      }
    }

    // Восстанавливаем фигуру только если поворот не удался
    if (!rotated) {
      for (int i = 0; i < kFigurePoints; ++i) {
        int x = current_figure->figure_points[i].point_x;
        int y = current_figure->figure_points[i].point_y;
        if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
          game_info->field[y][x] = 1;
        }
      }
    }
  }
}

// Handles the falling of the current tetromino.
void falling_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
                 FsmState *state) {
  GameState *gs = get_game_state();
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
      if (current_figure->figure_points[i].point_y >= 0 &&
          current_figure->figure_points[i].point_y < min_y) {
        min_y = current_figure->figure_points[i].point_y;
      }
    }
    gs->figure_y = min_y;
    // Перерисовка
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 1;
      }
    }
  } else {
    *state = LOCKING;
  }
}

// Handles the moving of the current tetromino left or right.
void moving_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
                FsmState *state, int *figure_x, UserAction direction) {
  bool can_move = true;
  int delta_x = (direction == kLeft) ? -1 : 1;

  int extreme_x[kRow];
  int points_per_y[kRow];
  for (int i = 0; i < kRow; ++i) {
    extreme_x[i] = (direction == kLeft) ? kCol : -1;
    points_per_y[i] = 0;
  }

  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (y >= 0 && y < kRow) {
      points_per_y[y]++;
      if (direction == kLeft) {
        if (x < extreme_x[y]) extreme_x[y] = x;
      } else {
        if (x > extreme_x[y]) extreme_x[y] = x;
      }
    }
  }

  for (int y = 0; y < kRow && can_move; ++y) {
    if (points_per_y[y] > 0) {
      int x = extreme_x[y] + delta_x;
      if (y < 0 || y >= kRow) {
        can_move = false;
      } else if (x < 0 || x >= kCol) {
        can_move = false;
      } else if (game_info->field[y][x]) {
        can_move = false;
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
  }
  *state = FALLING;
}

// Handles the clearing of completed lines.
void clearing_fsm(GameInfo *game_info, FsmState *state) {
  GameState *gs = get_game_state();
  int lines_cleared = 0;
  for (int y = kRow - 1; y >= 0 && lines_cleared < 4; --y) {
    bool line_full = true;
    for (int x = 0; x < kCol; ++x) {
      if (!game_info->field[y][x]) {
        line_full = false;
        break;
      }
    }
    if (line_full) {
      lines_cleared++;
      for (int yy = y; yy > 0; --yy) {
        for (int x = 0; x < kCol; ++x) {
          game_info->field[yy][x] = game_info->field[yy - 1][x];
        }
      }
      for (int x = 0; x < kCol; ++x) {
        game_info->field[0][x] = 0;
      }
      y++;
    }
  }

  if (lines_cleared > 0) {
    int points = 0;
    switch (lines_cleared) {
      case 1:
        points = 100;
        break;
      case 2:
        points = 300;
        break;
      case 3:
        points = 700;
        break;
      case 4:
        points = 1500;
        break;
    }
    game_info->score += points;
    gs->points_toward_level += points;

    // Проверяем повышение уровня
    while (gs->points_toward_level >= 600 && game_info->level < 10) {
      game_info->level++;
      gs->points_toward_level -= 600;
      game_info->speed = kSpeed - (game_info->level - 1) * 100;
      if (game_info->speed < 100) game_info->speed = 100;
    }

    // Обновляем high_score
    if (game_info->score > game_info->high_score) {
      game_info->high_score = game_info->score;
    }
  }

  *state = SPAWN;
}

// Sets the game to the game-over state.
void game_over_fsm(GameInfo *game_info) {
  game_info->pause = -1;
  FILE *file = fopen("high_score.txt", "w");
  if (file) {
    fprintf(file, "%d", game_info->high_score);
    fclose(file);
  }
}

// Processes user input to control the game.
void user_input(UserAction action, bool hold) {
  GameState *gs = get_game_state();
  GameInfo *info = &gs->game_info;
  switch (action) {
    case kStart:
      if (gs->state == START) {
        start_fsm(info);
        gs->state = SPAWN;
      }
      break;
    case kPause:
      info->pause = !info->pause;
      if (!info->pause && gs->state == PAUSED) {
        gs->state = FALLING;
      } else if (info->pause) {
        gs->state = PAUSED;
      }
      break;
    case kTerminate:
      gs->state = GAME_OVER;
      game_over_fsm(info);
      break;
    case kLeft:
    case kRight:
      if (!info->pause && gs->state == FALLING) {
        gs->move_direction = action;
        gs->state = MOVING;
      }
      break;
    case kUp:
      break;
    case kDown:
      if (!info->pause && (gs->state == FALLING || gs->state == MOVING)) {
        gs->state = FALLING;
        if (hold) {
          falling_fsm(info, &gs->current_figure, &gs->state);
        }
        while (gs->state == FALLING) {
          falling_fsm(info, &gs->current_figure, &gs->state);
        }
      }
      break;
    case kAction:
      if (!info->pause && gs->state == FALLING) {
        gs->state = ROTATING;
        rotate_figure(info, &gs->current_figure);
        gs->state = FALLING;
      }
      break;
  }
}

// Updates the game state and returns the current state for rendering.
GameInfo update_current_state() {
  GameState *gs = get_game_state();
  GameInfo *info = &gs->game_info;
  if (!info->pause && gs->state != GAME_OVER) {
    switch (gs->state) {
      case SPAWN:
        spawn_fsm(info, &gs->current_figure, &gs->figure_x, &gs->figure_y,
                  &gs->state);
        break;
      case FALLING:
        falling_fsm(info, &gs->current_figure, &gs->state);
        break;
      case MOVING:
        moving_fsm(info, &gs->current_figure, &gs->state, &gs->figure_x,
                   gs->move_direction);
        break;
      case LOCKING:
        gs->state = CLEARING;
        break;
      case CLEARING:
        clearing_fsm(info, &gs->state);
        break;
      default:
        break;
    }
  }
  return *info;
}

// Initializes and runs the Tetris game.
int run_tetris() {
  GameState *gs = get_game_state();
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
  free_matrix(gs->game_info.field, kRow);
  free_matrix(gs->game_info.next, kFigureSize);
  endwin();
  return 0;
}

int main() { return run_tetris(); }