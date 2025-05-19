#include <check.h>
#include <stdlib.h>

#include "../brick_game/tetris/tetris.h"

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
  gs->gameInfo.pause = false;
  gs->pointsTowardLevel = 0;
  gs->state = kStart;
  return gs;
}

/**
 * Cleans up the high score file before each test.
 */
void setup(void) { remove("brick_game/tetris/high_score.txt"); }

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
  ck_assert(info->pause);
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
  ck_assert(info->pause);
  ck_assert_int_eq(gs->state, kPaused);
  userInput(kActionPause, false);
  ck_assert_int_eq(gs->state, kFalling);
  userInput(kActionTerminate, false);
  ck_assert_int_eq(gs->state, kGameOver);
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
  userInput(kActionStart, false);                        // kStart -> kSpawn
  updateCurrentState();                                  // kSpawn -> kFalling
  gs->currentTetromino = spawnTetromino(info, 4, 0, 0, 0);  // I-tetromino
  gs->tetrominoX = 4;
  gs->tetrominoY = 0;
  gs->state = kFalling;
  userInput(kActionRight, false);
  ck_assert_int_eq(gs->state, kMoving);
  ck_assert_int_eq(gs->moveDirection, kActionRight);
  updateCurrentState();  // Process kMoving
  ck_assert_int_eq(gs->state, kFalling);
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
  ck_assert_int_eq(gs->state, kLocking);  // Single step
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
  info->pause = true;
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
  info->pause = false;
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