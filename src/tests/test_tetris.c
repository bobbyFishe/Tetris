#include <check.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../brick_game/tetris/tetris.h"

// Structure to track mvprintw calls
#define MAX_CALLS 1000
typedef struct {
  int y;
  int x;
  char str[256];
} MvprintwCall;

typedef struct {
  MvprintwCall calls[MAX_CALLS];
  int call_count;
  int refresh_count;
} NcursesMock;

NcursesMock mock = {0};

// Mock for mvprintw
int mvprintw(int y, int x, const char* fmt, ...) {
  if (mock.call_count >= MAX_CALLS) return -1;
  MvprintwCall* call = &mock.calls[mock.call_count++];
  call->y = y;
  call->x = x;
  va_list args;
  va_start(args, fmt);
  vsnprintf(call->str, sizeof(call->str), fmt, args);
  va_end(args);
  return 0;
}

// Mock for refresh
void mock_refresh(void) { mock.refresh_count++; }

// Reset mock before each test
void resetMock(void) {
  mock.call_count = 0;
  mock.refresh_count = 0;
}

/**
 * Initializes the game state for testing.
 * @return Pointer to the initialized GameState.
 */
GameState* initGameState() {
  GameState* gs = getGameState();
  gs->gameInfo.field = allocMatrix(kRow, kCol);
  gs->gameInfo.next = allocMatrix(kFigureSize, kFigureSize);
  gs->gameInfo.score = 0;
  gs->gameInfo.high_score = 0;
  gs->gameInfo.level = 1;
  gs->gameInfo.speed = kSpeed;
  gs->gameInfo.pause = 0;
  gs->pointsTowardLevel = 0;
  gs->state = kStart;
  return gs;
}

/**
 * Cleans up the high score file and resets mock before each test.
 */
void setup(void) {
  remove("brick_game/tetris/high_score.txt");
  resetMock();
}

/**
 * Tests the allocation of a matrix.
 */
START_TEST(testAllocMatrix) {
  int rows = 5, cols = 5;
  int** matrix = allocMatrix(rows, cols);
  ck_assert_ptr_nonnull(matrix);
  for (int i = 0; i < rows; ++i) {
    ck_assert_ptr_nonnull(matrix[i]);
    for (int j = 0; j < cols; ++j) {
      ck_assert_int_eq(matrix[i][j], 0);
    }
  }
  freeMatrix(matrix, rows);
}
END_TEST

/**
 * Tests the deallocation of a matrix.
 */
START_TEST(testFreeMatrix) {
  int rows = 5, cols = 5;
  int** matrix = allocMatrix(rows, cols);
  freeMatrix(matrix, rows);
  ck_assert(true);  // No crash indicates success.
}
END_TEST

/**
 * Tests spawning an I-tetromino.
 */
START_TEST(testSpawnTetromino) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  TetrominoPoints tetromino = spawnTetromino(info, 3, 0, 0, 0);  // I-tetromino
  int count = 0;
  for (int i = 0; i < kFigurePoints; ++i) {
    if (tetromino.points[i].x >= 0) ++count;
  }
  ck_assert_int_eq(count, kFigurePoints);
  ck_assert_int_eq(info->field[1][3], 1);
  ck_assert_int_eq(info->field[1][4], 1);
  ck_assert_int_eq(info->field[1][5], 1);
  ck_assert_int_eq(info->field[1][6], 1);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests generating the next tetromino.
 */
START_TEST(testGenerateNextTetromino) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  int tetrominoType, rotationIndex;
  generateNextTetromino(info, &tetrominoType, &rotationIndex);
  ck_assert_int_ge(tetrominoType, 0);
  ck_assert_int_lt(tetrominoType, 7);
  ck_assert_int_eq(rotationIndex, 0);
  ck_assert(hasNextTetromino(info));
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests checking the existence of the next tetromino.
 */
START_TEST(testHasNextTetromino) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  ck_assert(!hasNextTetromino(info));  // Empty next
  int tetrominoType, rotationIndex;
  generateNextTetromino(info, &tetrominoType, &rotationIndex);
  ck_assert(hasNextTetromino(info));  // Non-empty next
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests the start state of the finite state machine.
 */
START_TEST(testStartFsm) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  startGame(info);
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  ck_assert_int_eq(info->score, 0);
  ck_assert_int_eq(info->level, 1);
  ck_assert_int_eq(info->speed, kSpeed);
  ck_assert(!info->pause);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests the spawn state of the finite state machine.
 */
START_TEST(testSpawnFsm) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  TetrominoPoints tetromino;
  int tetrominoX, tetrominoY;
  FsmState state = kSpawn;
  spawnTetrominoState(info, &tetromino, &tetrominoX, &tetrominoY, &state);
  ck_assert_int_eq(state, kFalling);
  ck_assert_int_eq(tetrominoX, kCol / 2 - kFigureSize / 2);
  ck_assert_int_eq(tetrominoY, 0);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests the falling state of the finite state machine.
 */
START_TEST(testFallingFsm) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  TetrominoPoints tetromino =
      spawnTetromino(info, 3, kRow - 2, 0, 0);  // I-tetromino near bottom
  gs->currentTetromino = tetromino;
  gs->tetrominoX = 3;
  gs->tetrominoY = kRow - 2;
  FsmState state = kFalling;
  fallingTetrominoState(info, &tetromino, &state);
  ck_assert_int_eq(state, kLocking);  // Hits bottom
  ck_assert_int_eq(tetromino.points[0].y, kRow - 1);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests the moving state of the finite state machine.
 */
START_TEST(testMovingFsm) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  TetrominoPoints tetromino = spawnTetromino(info, 3, 0, 0, 0);  // I-tetromino
  gs->currentTetromino = tetromino;
  gs->tetrominoX = 3;
  FsmState state = kMoving;
  int tetrominoX = 3;
  movingTetrominoState(info, &tetromino, &state, &tetrominoX, kActionRight);
  ck_assert_int_eq(state, kFalling);
  ck_assert_int_eq(tetrominoX, 4);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests rotating an I-tetromino.
 */
START_TEST(testRotateTetromino) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  gs->tetrominoType = 0;  // I-tetromino
  gs->rotationIndex = 0;
  gs->tetrominoX = 3;
  gs->tetrominoY = 0;
  TetrominoPoints tetromino = spawnTetromino(info, 3, 0, 0, 0);
  gs->currentTetromino = tetromino;
  rotateTetromino(info, &tetromino);
  ck_assert_int_eq(gs->rotationIndex, 1);
  ck_assert_int_eq(tetromino.points[0].y, 0);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests the clearing state of the finite state machine.
 */
START_TEST(testClearingFsm) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int x = 0; x < kCol; ++x) {
    info->field[kRow - 1][x] = 1;  // Fill bottom row
  }
  FsmState state = kClearing;
  clearLinesState(info, &state);
  ck_assert_int_eq(state, kSpawn);
  ck_assert_int_eq(info->score, kScoreSingleLine);
  ck_assert_int_eq(info->level, 1);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests the game over state of the finite state machine.
 */
START_TEST(testGameOverFsm) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  info->score = 500;
  info->high_score = 200;
  gameOverState(info);
  ck_assert_int_eq(info->pause, -1);
  ck_assert_int_eq(info->high_score, 500);  // Updated high score
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests user input for start, pause, and terminate actions.
 */
START_TEST(testUserInput) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  userInput(kActionStart, false);
  ck_assert_int_eq(gs->state, kSpawn);
  userInput(kActionPause, false);
  ck_assert_int_eq(info->pause, 1);
  ck_assert_int_eq(gs->state, kPaused);
  userInput(kActionPause, false);
  ck_assert_int_eq(info->pause, 0);
  ck_assert_int_eq(gs->state, kFalling);
  userInput(kActionTerminate, false);
  ck_assert_int_eq(gs->state, kGameOver);
  ck_assert_int_eq(info->pause, -1);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests user input for movement and rotation actions.
 */
START_TEST(testUserInputMovement) {
  // Test kActionRight: kFalling -> kMoving -> kFalling
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;  // Clear field
    }
  }
  userInput(kActionStart, false);  // kStart -> kSpawn
  updateCurrentState();            // kSpawn -> kFalling
  gs->currentTetromino = spawnTetromino(info, 4, 0, 0, 0);  // I-tetromino
  gs->tetrominoX = 4;
  gs->tetrominoY = 0;
  gs->state = kFalling;
  userInput(kActionRight, false);
  ck_assert_int_eq(gs->state, kMoving);
  ck_assert_int_eq(gs->moveDirection, kActionRight);
  updateCurrentState();  // Process kMoving
  ck_assert_int_eq(gs->state, kLocking);
  ck_assert_int_eq(gs->tetrominoX, 5);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test kActionLeft: kFalling -> kMoving -> kFalling
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  userInput(kActionStart, false);
  updateCurrentState();
  gs->currentTetromino = spawnTetromino(info, 4, 0, 0, 0);
  gs->tetrominoX = 4;
  gs->tetrominoY = 0;
  gs->state = kFalling;
  userInput(kActionLeft, false);
  ck_assert_int_eq(gs->state, kMoving);
  ck_assert_int_eq(gs->moveDirection, kActionLeft);
  updateCurrentState();
  ck_assert_int_eq(gs->state, kFalling);
  ck_assert_int_eq(gs->tetrominoX, 3);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test kActionDown (hold = false): Single step down
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  userInput(kActionStart, false);
  updateCurrentState();
  gs->currentTetromino = spawnTetromino(info, 4, 0, 0, 0);
  gs->tetrominoX = 4;
  gs->tetrominoY = 0;
  gs->state = kFalling;
  userInput(kActionDown, false);
  ck_assert_int_eq(gs->state, kLocking);
  updateCurrentState();
  ck_assert_int_eq(gs->tetrominoY, 0);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test kActionDown (hold = true): Drop to bottom
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  userInput(kActionStart, false);
  updateCurrentState();
  gs->currentTetromino = spawnTetromino(info, 4, 0, 0, 0);
  gs->tetrominoX = 4;
  gs->tetrominoY = 0;
  gs->state = kFalling;
  userInput(kActionDown, true);
  ck_assert_int_eq(gs->state, kLocking);
  ck_assert_int_eq(gs->currentTetromino.points[0].y, kRow - 1);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test kActionRotate: Rotate figure
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  userInput(kActionStart, false);
  updateCurrentState();
  gs->currentTetromino = spawnTetromino(info, 4, 2, 0, 0);  // I-tetromino
  gs->tetrominoX = 4;
  gs->tetrominoY = 2;
  gs->tetrominoType = 0;
  gs->rotationIndex = 0;
  gs->state = kFalling;
  userInput(kActionRotate, false);
  ck_assert_int_eq(gs->state, kFalling);
  ck_assert_int_eq(gs->rotationIndex, 1);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test ignoring actions when paused
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  userInput(kActionStart, false);
  updateCurrentState();
  info->pause = 1;  // Paused
  gs->state = kFalling;
  userInput(kActionRight, false);
  ck_assert_int_eq(gs->state, kFalling);  // No change
  userInput(kActionLeft, false);
  ck_assert_int_eq(gs->state, kFalling);
  userInput(kActionDown, true);
  ck_assert_int_eq(gs->state, kFalling);
  userInput(kActionRotate, false);
  ck_assert_int_eq(gs->state, kFalling);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test ignoring actions in kStart
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  info->pause = 0;
  gs->state = kStart;
  userInput(kActionRight, false);
  ck_assert_int_eq(gs->state, kStart);
  userInput(kActionLeft, false);
  ck_assert_int_eq(gs->state, kStart);
  userInput(kActionDown, true);
  ck_assert_int_eq(gs->state, kStart);
  userInput(kActionRotate, false);
  ck_assert_int_eq(gs->state, kStart);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);

  // Test ignoring actions when game over
  gs = initGameState();
  info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      info->field[i][j] = 0;
    }
  }
  userInput(kActionStart, false);
  updateCurrentState();
  info->pause = -1;  // Game over
  gs->state = kGameOver;
  userInput(kActionRight, false);
  ck_assert_int_eq(gs->state, kGameOver);  // No change
  userInput(kActionLeft, false);
  ck_assert_int_eq(gs->state, kGameOver);
  userInput(kActionDown, true);
  ck_assert_int_eq(gs->state, kGameOver);
  userInput(kActionRotate, false);
  ck_assert_int_eq(gs->state, kGameOver);
  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests renderField with an empty field and active game.
 */
START_TEST(testRenderFieldActive) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);

  // Set up game info
  info->score = 100;
  info->high_score = 500;
  info->level = 2;
  info->pause = 0;
  // Set I-tetromino in next
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      info->next[i][j] = kTetrominoShapes[0][0][i][j];  // I-tetromino
    }
  }

  renderField(*info);

  // Check field rendering (empty field)
  int call_idx = 0;
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      ck_assert_int_eq(mock.calls[call_idx].y, i);
      ck_assert_int_eq(mock.calls[call_idx].x, j);
      ck_assert_str_eq(mock.calls[call_idx].str, " ");
      call_idx++;
    }
  }

  // Check right border
  for (int i = 0; i < kRow; ++i) {
    ck_assert_int_eq(mock.calls[call_idx].y, i);
    ck_assert_int_eq(mock.calls[call_idx].x, kCol);
    ck_assert_str_eq(mock.calls[call_idx].str, "|");
    call_idx++;
  }

  // Check bottom border
  for (int i = 0; i < kCol + 1; ++i) {
    ck_assert_int_eq(mock.calls[call_idx].y, kRow);
    ck_assert_int_eq(mock.calls[call_idx].x, i);
    ck_assert_str_eq(mock.calls[call_idx].str, "-");
    call_idx++;
  }

  // Check "Next" label
  ck_assert_int_eq(mock.calls[call_idx].y, 0);
  ck_assert_int_eq(mock.calls[call_idx].x, kCol + 3);
  ck_assert_str_eq(mock.calls[call_idx].str, "Next");
  call_idx++;

  // Check next tetromino (I-tetromino)
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      ck_assert_int_eq(mock.calls[call_idx].y, i + 2);
      ck_assert_int_eq(mock.calls[call_idx].x, j + kCol + 3);
      ck_assert_str_eq(mock.calls[call_idx].str,
                       kTetrominoShapes[0][0][i][j] ? "o" : " ");
      call_idx++;
    }
  }

  // Check stats
  ck_assert_int_eq(mock.calls[call_idx].y, 6);
  ck_assert_int_eq(mock.calls[call_idx].x, kCol + 3);
  ck_assert_str_eq(mock.calls[call_idx].str, "Level: 2");
  call_idx++;

  ck_assert_int_eq(mock.calls[call_idx].y, 7);
  ck_assert_int_eq(mock.calls[call_idx].x, kCol + 3);
  ck_assert_str_eq(mock.calls[call_idx].str, "Score: 100");
  call_idx++;

  ck_assert_int_eq(mock.calls[call_idx].y, 8);
  ck_assert_int_eq(mock.calls[call_idx].x, kCol + 3);
  ck_assert_str_eq(mock.calls[call_idx].str, "High Score: 500");
  call_idx++;

  // No pause or game over message
  ck_assert_int_eq(mock.call_count, call_idx);

  // Check refresh
  ck_assert_int_eq(mock.refresh_count, 0);

  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests renderField with a paused game.
 */
START_TEST(testRenderFieldPaused) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);

  // Set up game info
  info->score = 0;
  info->high_score = 0;
  info->level = 1;
  info->pause = 1;  // Paused

  renderField(*info);

  // Skip checking field, borders, next, and stats (same as active)
  int call_idx =
      kRow * kCol + kRow + (kCol + 1) + 1 + kFigureSize * kFigureSize + 3;

  // Check "PAUSED" message
  ck_assert_int_eq(mock.calls[call_idx].y, kRow / 2);
  ck_assert_int_eq(mock.calls[call_idx].x, kCol / 2 - 3);
  ck_assert_str_eq(mock.calls[call_idx].str, "PAUSED");
  call_idx++;

  // No game over message
  ck_assert_int_eq(mock.call_count, call_idx);

  // Check refresh
  ck_assert_int_eq(mock.refresh_count, 0);

  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests renderField with game over.
 */
START_TEST(testRenderFieldGameOver) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);

  // Set up game info
  info->score = 0;
  info->high_score = 0;
  info->level = 1;
  info->pause = -1;  // Game over

  renderField(*info);

  // Skip checking field, borders, next, and stats
  int call_idx =
      kRow * kCol + kRow + (kCol + 1) + 1 + kFigureSize * kFigureSize + 3;

  // Check "GAME OVER" message
  ck_assert_int_eq(mock.calls[call_idx].y, kRow / 2);
  ck_assert_int_eq(mock.calls[call_idx].x, kCol / 2 - 5);
  ck_assert_str_eq(mock.calls[call_idx].str, "GAME OVER");
  call_idx++;

  // No pause message
  ck_assert_int_eq(mock.call_count, call_idx);

  // Check refresh
  ck_assert_int_eq(mock.refresh_count, 0);

  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Tests renderField with a non-empty field.
 */
START_TEST(testRenderFieldNonEmpty) {
  GameState* gs = initGameState();
  GameInfo* info = &gs->gameInfo;
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);

  // Set up game info
  info->score = 0;
  info->high_score = 0;
  info->level = 1;
  info->pause = 0;
  // Place I-tetromino on field
  spawnTetromino(info, 3, 0, 0, 0);  // I-tetromino

  renderField(*info);

  // Check field rendering (I-tetromino at y=1, x=3,4,5,6)
  int call_idx = 0;
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      ck_assert_int_eq(mock.calls[call_idx].y, i);
      ck_assert_int_eq(mock.calls[call_idx].x, j);
      if (i == 1 && (j >= 3 && j <= 6)) {
        ck_assert_str_eq(mock.calls[call_idx].str, "o");
      } else {
        ck_assert_str_eq(mock.calls[call_idx].str, " ");
      }
      call_idx++;
    }
  }

  // Skip checking borders, next, and stats (same as active)
  call_idx =
      kRow * kCol + kRow + (kCol + 1) + 1 + kFigureSize * kFigureSize + 3;

  // No pause or game over message
  ck_assert_int_eq(mock.call_count, call_idx);

  // Check refresh
  ck_assert_int_eq(mock.refresh_count, 0);

  freeMatrix(info->field, kRow);
  freeMatrix(info->next, kFigureSize);
}
END_TEST

/**
 * Creates the test suite for Tetris.
 * @return Pointer to the test suite.
 */
Suite* tetrisSuite(void) {
  Suite* s = suite_create("Tetris");
  TCase* tc_core = tcase_create("Core");
  tcase_add_checked_fixture(tc_core, setup, NULL);
  tcase_add_test(tc_core, testAllocMatrix);
  tcase_add_test(tc_core, testFreeMatrix);
  tcase_add_test(tc_core, testSpawnTetromino);
  tcase_add_test(tc_core, testGenerateNextTetromino);
  tcase_add_test(tc_core, testHasNextTetromino);
  tcase_add_test(tc_core, testStartFsm);
  tcase_add_test(tc_core, testSpawnFsm);
  tcase_add_test(tc_core, testFallingFsm);
  tcase_add_test(tc_core, testMovingFsm);
  tcase_add_test(tc_core, testRotateTetromino);
  tcase_add_test(tc_core, testClearingFsm);
  tcase_add_test(tc_core, testGameOverFsm);
  tcase_add_test(tc_core, testUserInput);
  tcase_add_test(tc_core, testUserInputMovement);
  tcase_add_test(tc_core, testRenderFieldActive);
  tcase_add_test(tc_core, testRenderFieldPaused);
  tcase_add_test(tc_core, testRenderFieldGameOver);
  tcase_add_test(tc_core, testRenderFieldNonEmpty);
  suite_add_tcase(s, tc_core);
  return s;
}

/**
 * Runs the Tetris test suite.
 * @return 0 if all tests pass, 1 if any test fails.
 */
int main(void) {
  Suite* s = tetrisSuite();
  SRunner* sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int nf = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (nf == 0) ? 0 : 1;
}