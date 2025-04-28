#ifndef TETRIS_H_
#define TETRIS_H_

#include <stdbool.h>

// Constants for game field dimensions and settings.
enum {
  kRow = 20,
  kCol = 10,
  kFigureSize = 4,
  kSpeed = 1000,
  kFigurePoints = 4
};

// States of the game finite state machine.
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
} FsmState;

// User input actions.
typedef enum {
  kStart,
  kPause,
  kTerminate,
  kLeft,
  kRight,
  kUp,
  kDown,
  kAction
} UserAction;

// Represents a single point of a tetromino.
typedef struct {
  int point_x;
  int point_y;
} FigurePoint;

// Stores the points of the current tetromino.
typedef struct {
  FigurePoint figure_points[kFigurePoints];
} CurrentFigurePoints;

// Game state information for rendering.
typedef struct {
  int **field;
  int **next;
  int score;
  int high_score;
  int level;
  int speed;
  int pause;
} GameInfo;

// Tetromino shapes with all rotations.
// Order: I (2), L (4), O (1), T (4), S (2), Z (2), J (4).
static const int kTetrominoes[][4][kFigureSize][kFigureSize] = {
  // I: Horizontal, Vertical
  {{{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
  // L: 0°, 90°, 180°, 270°
  {{{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 1, 1, 1}, {0, 1, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}},
   {{0, 0, 0, 0}, {0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},
  // O: No rotation
  {{{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
  // T: 0°, 90°, 180°, 270°
  {{{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
   {{0, 1, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}},
  // S: 0°/180°, 90°/270°
  {{{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 1, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
  // Z: 0°/180°, 90°/270°
  {{{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
  // J: 0°, 90°, 180°, 270°
  {{{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 1}, {0, 0, 0, 0}},
   {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}},
   {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}}}
};

// Internal game state.
typedef struct {
  FsmState state;
  int figure_x;
  int figure_y;
  int figure_type;
  int next_figure_type;
  int rotation_idx;
  UserAction move_direction;
  CurrentFigurePoints current_figure;
  GameInfo game_info;
} GameState;

// Initializes and runs the Tetris game.
// Returns 0 on successful termination.
int run_tetris();

// Processes user input to control the game.
// Parameters:
//   action - The user action (e.g., move left, pause).
//   hold - Whether the action is held (currently unused).
void user_input(UserAction action, bool hold);

// Updates the game state and returns the current state for rendering.
// Returns:
//   The current game state.
GameInfo update_current_state();

int **alloc_matrix(int rows, int cols);
void free_matrix(int **matrix, int rows);
bool rotate_figure(GameInfo *game_info, CurrentFigurePoints *current_figure);
void next_figure_generate(GameInfo *game_info, int *figure_type, int *rotation_idx);
int next_check(const GameInfo *game_info);
void print_field(const GameInfo game_info);
void start_fsm(GameInfo *game_info);
void spawn_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
    int *figure_x, int *figure_y, FsmState *state);
void falling_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
        FsmState *state);
void moving_fsm(GameInfo *game_info, CurrentFigurePoints *current_figure,
          FsmState *state, int *figure_x, UserAction direction);
void game_over_fsm(GameInfo *game_info);
void user_input(UserAction action, bool hold);
CurrentFigurePoints figure_spawn(GameInfo *game_info, int figure_x, int figure_y,
  int figure_type, int rotation_idx);

#endif  // TETRIS_H_