#include "tetris.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Number of rotations per tetromino type (I, L, O, T, S, Z, J).
static const int kRotationsPerTetromino[] = {2, 4, 1, 4, 2, 2, 4};
static GameState game_state = {0};

// Allocates a matrix of given dimensions.
// Parameters:
//   rows - Number of rows.
//   cols - Number of columns.
// Returns:
//   Pointer to the allocated matrix, or NULL on failure.
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
// Parameters:
//   matrix - The matrix to free.
//   rows - Number of rows.
void free_matrix(int **matrix, int rows) {
  if (matrix) {
    for (int i = 0; i < rows; ++i) {
      free(matrix[i]);
    }
    free(matrix);
  }
}

// Spawns a new tetromino on the field using the current rotation.
// Parameters:
//   game_info - Pointer to the game state.
//   figure_x - X-coordinate of the tetromino's top-left corner.
//   figure_y - Y-coordinate of the tetromino's top-left corner.
//   figure_type - Type of tetromino (0-6).
//   rotation_idx - Current rotation index.
// Returns:
//   The points of the spawned tetromino.
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
    for (; count < kFigurePoints; ++count) {
      figure.figure_points[count].point_x = -1;
      figure.figure_points[count].point_y = -1;
    }
  }
  return figure;
}

// Generates a new tetromino for the next slot.
// Parameters:
//   game_info - Pointer to the game state.
//   figure_type - Pointer to store the tetromino type.
//   rotation_idx - Pointer to store the initial rotation index.
void next_figure_generate(GameInfo *game_info, int *figure_type, int *rotation_idx) {
  *figure_type = rand() % (sizeof(kTetrominoes) / sizeof(kTetrominoes[0]));
  *rotation_idx = 0; // Start with default rotation.
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      game_info->next[i][j] = kTetrominoes[*figure_type][*rotation_idx][i][j];
    }
  }
}

// Checks if the next tetromino exists.
// Parameters:
//   game_info - Pointer to the game state.
// Returns:
//   1 if a tetromino exists, 0 otherwise.
int next_check(const GameInfo *game_info) {
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      if (game_info->next[i][j]) return 1;
    }
  }
  return 0;
}

// Renders the game field and UI.
// Parameters:
//   game_info - The game state to render.
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
// Parameters:
//   game_info - Pointer to the game state to initialize.
void start_fsm(GameInfo *game_info) {
  game_info->field = alloc_matrix(kRow, kCol);
  game_info->next = alloc_matrix(kFigureSize, kFigureSize);
  game_info->score = 0;
  game_info->high_score = 0;
  game_info->level = 1;
  game_info->speed = kSpeed;
  game_info->pause = 0;
}

// Handles the spawning of a new tetromino.
// Parameters:
//   game_info - Pointer to the game state.
//   current_figure - Pointer to the current tetromino's points.
//   figure_x - Pointer to the tetromino's x-coordinate.
//   figure_y - Pointer to the tetromino's y-coordinate.
//   state - Pointer to the game state.
void spawn_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
               int *figure_x, int *figure_y, FsmState *state) {
  *figure_x = kCol / 2 - 2;
  *figure_y = 0;

  // Use next_figure_type if available, otherwise generate a new one.
  if (!next_check(game_info)) {
    next_figure_generate(game_info, &game_state.next_figure_type, &game_state.rotation_idx);
  }

  // Set current figure type to next figure type.
  game_state.figure_type = game_state.next_figure_type;
  game_state.rotation_idx = 0; // Reset rotation for new figure.

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
          }
        }
      }
    }
  }
  if (!collision) {
    *current_figure = figure_spawn(game_info, *figure_x, *figure_y,
                                  game_state.figure_type, game_state.rotation_idx);
    // Generate next figure type.
    next_figure_generate(game_info, &game_state.next_figure_type, &game_state.rotation_idx);
    *state = FALLING;
  }

  // Debug: Print spawn info.
  mvprintw(10, kCol + 2, "Spawn: Type=%d, Next=%d, Rot=%d, Pos=(%d,%d)",
           game_state.figure_type, game_state.next_figure_type, game_state.rotation_idx,
           *figure_x, *figure_y);
  refresh();
}

// Rotates the current tetromino clockwise by switching to the next rotation.
// Parameters:
//   game_info - Pointer to the game state.
//   current_figure - Pointer to the current tetromino's points.
// Returns:
//   True if rotation succeeded, false if blocked.
bool rotate_figure(GameInfo *game_info, CurrentFigurePoints *current_figure) {
  // Get current tetromino type and rotation.
  int figure_type = game_state.figure_type;
  int current_rotation = game_state.rotation_idx;
  int num_rotations = kRotationsPerTetromino[figure_type];

  // O-tetromino doesn't rotate.
  if (num_rotations == 1) {
    mvprintw(0, kCol + 2, "O-shape: No rotation");
    refresh();
    return true;
  }

  // Try next rotation with wall kicks.
  int next_rotation = (current_rotation + 1) % num_rotations;
  const int (*tetromino)[kFigureSize] = kTetrominoes[figure_type][next_rotation];
  int offsets[7][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {2, 0}, {-2, 0}};
  int num_offsets = (figure_type == 0) ? 7 : 5; // More offsets for I-tetromino.

  // Clear current figure from field to avoid self-collision.
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      game_info->field[y][x] = 0;
    }
  }

  // Debug: Print current state before rotation.
  mvprintw(0, kCol + 2, "Try rotate: Type=%d, Rot=%d", figure_type, next_rotation);
  mvprintw(1, kCol + 2, "Pos: (%d, %d)", game_state.figure_x, game_state.figure_y);
  for (int i = 0; i < kFigurePoints; ++i) {
    mvprintw(2 + i, kCol + 2, "Point %d: (%d, %d)", i,
             current_figure->figure_points[i].point_x,
             current_figure->figure_points[i].point_y);
  }

  for (int k = 0; k < num_offsets; ++k) {
    int offset_x = offsets[k][0];
    int offset_y = offsets[k][1];
    int new_figure_x = game_state.figure_x + offset_x;
    int new_figure_y = game_state.figure_y + offset_y;

    // Check if new rotation is valid.
    bool valid = true;
    int fail_x = -1, fail_y = -1;
    bool is_collision = false;
    for (int i = 0; i < kFigureSize && valid; ++i) {
      for (int j = 0; j < kFigureSize && valid; ++j) {
        if (tetromino[i][j]) {
          int new_x = j + new_figure_x;
          int new_y = i + new_figure_y;
          if (new_x < 0 || new_x >= kCol || new_y < 0 || new_y >= kRow) {
            valid = false;
            fail_x = new_x;
            fail_y = new_y;
          } else if (game_info->field[new_y][new_x]) {
            valid = false;
            fail_x = new_x;
            fail_y = new_y;
            is_collision = true;
          }
        }
      }
    }

    if (valid) {
      // Apply new rotation and position.
      game_state.rotation_idx = next_rotation;
      game_state.figure_x = new_figure_x;
      game_state.figure_y = new_figure_y;
      *current_figure = figure_spawn(game_info, new_figure_x, new_figure_y,
                                    figure_type, next_rotation);

      // Debug: Print rotation success.
      mvprintw(0, kCol + 2, "Rotated: Type=%d, Rot=%d", figure_type, next_rotation);
      mvprintw(1, kCol + 2, "Pos: (%d, %d), Offset: (%d, %d)", new_figure_x, new_figure_y, offset_x, offset_y);
      for (int i = 0; i < kFigurePoints; ++i) {
        mvprintw(2 + i, kCol + 2, "Point %d: (%d, %d)", i,
                 current_figure->figure_points[i].point_x,
                 current_figure->figure_points[i].point_y);
      }
      refresh();
      return true;
    }

    // Debug: Print failure reason for this offset.
    mvprintw(6 + k, kCol + 2, "Offset (%d, %d): %s at (%d, %d)",
             offset_x, offset_y, is_collision ? "Collision" : "Out of bounds", fail_x, fail_y);
  }

  // Restore current figure if rotation failed.
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = current_figure->figure_points[i].point_x;
    int y = current_figure->figure_points[i].point_y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      game_info->field[y][x] = 1;
    }
  }

  // Debug: Print overall failure.
  mvprintw(0, kCol + 2, "Rotation failed: Type=%d, Rot=%d", figure_type, next_rotation);
  mvprintw(1, kCol + 2, "Pos: (%d, %d)", game_state.figure_x, game_state.figure_y);
  refresh();
  return false;
}

// Handles the falling of the current tetromino.
// Parameters:
//   game_info - Pointer to the game state.
//   current_figure - Pointer to the current tetromino's points.
//   state - Pointer to the game state.
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
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 0;
      }
    }
    for (int i = 0; i < kFigurePoints; ++i) {
      if (current_figure->figure_points[i].point_x >= 0) {
        current_figure->figure_points[i].point_y++;
      }
    }
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = current_figure->figure_points[i].point_x;
      int y = current_figure->figure_points[i].point_y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        game_info->field[y][x] = 1;
      }
    }
    game_state.figure_y++;
  } else {
    *state = LOCKING;
  }
}

// Sets the game to the game-over state.
// Parameters:
//   game_info - Pointer to the game state.
void game_over_fsm(GameInfo *game_info) {
  game_info->pause = -1;
}

// Processes user input to control the game.
// Parameters:
//   action - The user action (e.g., move left, pause).
//   hold - Whether the action is held.
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
        game_state.state = MOVING;
        // TODO: Implement MOVING logic.
        game_state.state = FALLING;
      }
      break;
    case kUp:
      break;
    case kDown:
      if (!info->pause && (game_state.state == FALLING || game_state.state == MOVING)) {
        game_state.state = FALLING;
        if (hold) {
          // Instant drop to bottom.
          while (game_state.state == FALLING) {
            falling_fsm(info, &game_state.current_figure, &game_state.state);
          }
        } else {
          // Faster fall (e.g., double speed).
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
  bool key_held = false;
  int last_key = -1;
  int down_press_count = 0;
  while (true) {
    int ch = getch();
    if (ch == ERR) {
      // No key pressed; check if last key is held.
      if (key_held && last_key == KEY_DOWN) {
        user_input(kDown, true);  // Instant drop for hold.
      }
    } else {
      // Key pressed; update hold state.
      key_held = (ch == last_key);
      last_key = ch;
      if (ch == 'q') {
        user_input(kTerminate, false);
      } else if (ch == 'p') {
        user_input(kPause, false);
      } else if (ch == KEY_LEFT) {
        user_input(kLeft, false);
      } else if (ch == KEY_RIGHT) {
        user_input(kRight, false);
      } else if (ch == KEY_DOWN) {
        down_press_count++;
        user_input(kDown, false);  // Always process down for smoother fast fall.
      } else if (ch == ' ') {
        user_input(kAction, false);
      }
    }

    // Detect key release.
    if (ch == ERR && last_key != -1) {
      key_held = false;
      last_key = -1;
      down_press_count = 0;
    }

    GameInfo state = update_current_state();
    if (state.pause == -1) break;

    clear();
    print_field(state);
    napms(state.speed / 2);  // Faster updates for responsive input.
  }

  free_matrix(game_state.game_info.field, kRow);
  free_matrix(game_state.game_info.next, kFigureSize);
  endwin();
  return 0;
}

int main() {
  return run_tetris();
}