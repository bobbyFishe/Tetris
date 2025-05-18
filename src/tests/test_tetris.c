#include <check.h>
#include <stdlib.h>

#include "../brick_game/tetris/tetris.h"

// Helper function to initialize game state
GameState *init_game_state() {
  GameState *gs = get_game_state();
  gs->game_info.field = alloc_matrix(kRow, kCol);
  gs->game_info.next = alloc_matrix(kFigureSize, kFigureSize);
  gs->game_info.score = 0;
  gs->game_info.high_score = 0;
  gs->game_info.level = 1;
  gs->game_info.speed = kSpeed;
  gs->game_info.pause = 0;
  gs->points_toward_level = 0;
  gs->state = START;
  return gs;
}

// Clean up high_score.txt before each test
void setup(void) { remove("brick_game/tetris/high_score.txt"); }

// Test matrix allocation
START_TEST(test_alloc_matrix) {
  int rows = 5, cols = 5;
  int **matrix = alloc_matrix(rows, cols);
  ck_assert_ptr_nonnull(matrix);
  for (int i = 0; i < rows; i++) {
    ck_assert_ptr_nonnull(matrix[i]);
    for (int j = 0; j < cols; j++) {
      ck_assert_int_eq(matrix[i][j], 0);
    }
  }
  free_matrix(matrix, rows);
}
END_TEST

// Test matrix deallocation
START_TEST(test_free_matrix) {
  int rows = 5, cols = 5;
  int **matrix = alloc_matrix(rows, cols);
  free_matrix(matrix, rows);
  ck_assert(true);
}
END_TEST

// Test figure spawn
START_TEST(test_figure_spawn) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  CurrentFigurePoints figure = figure_spawn(info, 3, 0, 0, 0);  // I-tetromino
  int count = 0;
  for (int i = 0; i < kFigurePoints; i++) {
    if (figure.figure_points[i].point_x >= 0) count++;
  }
  ck_assert_int_eq(count, kFigurePoints);
  ck_assert_int_eq(info->field[1][3], 1);
  ck_assert_int_eq(info->field[1][4], 1);
  ck_assert_int_eq(info->field[1][5], 1);
  ck_assert_int_eq(info->field[1][6], 1);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test next figure generation
START_TEST(test_next_figure_generate) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  int figure_type, rotation_idx;
  next_figure_generate(info, &figure_type, &rotation_idx);
  ck_assert_int_ge(figure_type, 0);
  ck_assert_int_lt(figure_type, 7);
  ck_assert_int_eq(rotation_idx, 0);
  ck_assert_int_eq(next_check(info), 1);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test next check
START_TEST(test_next_check) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  ck_assert_int_eq(next_check(info), 0);  // Empty next
  int figure_type, rotation_idx;
  next_figure_generate(info, &figure_type, &rotation_idx);
  ck_assert_int_eq(next_check(info), 1);  // Non-empty next
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test start FSM
START_TEST(test_start_fsm) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  start_fsm(info);
  ck_assert_ptr_nonnull(info->field);
  ck_assert_ptr_nonnull(info->next);
  ck_assert_int_eq(info->score, 0);
  ck_assert_int_eq(info->level, 1);
  ck_assert_int_eq(info->speed, kSpeed);
  ck_assert_int_eq(info->pause, 0);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test spawn FSM
START_TEST(test_spawn_fsm) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  CurrentFigurePoints figure;
  int figure_x, figure_y;
  FsmState state = SPAWN;
  spawn_fsm(info, &figure, &figure_x, &figure_y, &state);
  ck_assert_int_eq(state, FALLING);
  ck_assert_int_eq(figure_x, kCol / 2 - kFigureSize / 2);
  ck_assert_int_eq(figure_y, 0);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test falling FSM
START_TEST(test_falling_fsm) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  CurrentFigurePoints figure =
      figure_spawn(info, 3, kRow - 2, 0, 0);  // I-tetromino near bottom
  gs->current_figure = figure;
  gs->figure_x = 3;
  gs->figure_y = kRow - 2;
  FsmState state = FALLING;
  falling_fsm(info, &figure, &state);
  ck_assert_int_eq(state, LOCKING);  // Should hit bottom
  ck_assert_int_eq(figure.figure_points[0].point_y, kRow - 1);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test moving FSM
START_TEST(test_moving_fsm) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  CurrentFigurePoints figure = figure_spawn(info, 3, 0, 0, 0);  // I-tetromino
  gs->current_figure = figure;
  gs->figure_x = 3;
  FsmState state = MOVING;
  int figure_x = 3;
  moving_fsm(info, &figure, &state, &figure_x, kRight);
  ck_assert_int_eq(state, FALLING);
  ck_assert_int_eq(figure_x, 4);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test rotate figure
START_TEST(test_rotate_figure) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  gs->figure_type = 0;  // I-tetromino
  gs->rotation_idx = 0;
  gs->figure_x = 3;
  gs->figure_y = 0;
  CurrentFigurePoints figure = figure_spawn(info, 3, 0, 0, 0);
  gs->current_figure = figure;
  rotate_figure(info, &figure);
  ck_assert_int_eq(gs->rotation_idx, 1);
  ck_assert_int_eq(figure.figure_points[0].point_y, 0);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test clearing FSM
START_TEST(test_clearing_fsm) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  for (int x = 0; x < kCol; x++) info->field[kRow - 1][x] = 1;
  FsmState state = CLEARING;
  clearing_fsm(info, &state);
  ck_assert_int_eq(state, SPAWN);
  ck_assert_int_eq(info->score, 100);
  ck_assert_int_eq(info->level, 1);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test game over FSM
START_TEST(test_game_over_fsm) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  info->score = 500;
  info->high_score = 200;
  game_over_fsm(info);
  ck_assert_int_eq(info->pause, -1);
  ck_assert_int_eq(info->high_score, 200);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test user input for start, pause, terminate
START_TEST(test_user_input) {
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  user_input(kStart, false);
  ck_assert_int_eq(gs->state, SPAWN);
  user_input(kPause, false);
  ck_assert_int_eq(info->pause, 1);
  ck_assert_int_eq(gs->state, PAUSED);
  user_input(kPause, false);
  ck_assert_int_eq(gs->state, FALLING);
  user_input(kTerminate, false);
  ck_assert_int_eq(gs->state, GAME_OVER);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

// Test user input for movement and rotation
START_TEST(test_user_input_movement) {
  // Test kRight: FALLING -> MOVING -> FALLING
  GameState *gs = init_game_state();
  GameInfo *info = &gs->game_info;
  // Clear field explicitly
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  user_input(kStart, false);                            // START -> SPAWN
  update_current_state();                               // SPAWN -> FALLING
  gs->current_figure = figure_spawn(info, 4, 0, 0, 0);  // I-tetromino
  gs->figure_x = 4;
  gs->figure_y = 0;
  gs->state = FALLING;
  user_input(kRight, false);
  ck_assert_int_eq(gs->state, MOVING);
  ck_assert_int_eq(gs->move_direction, kRight);
  update_current_state();  // Process MOVING
  ck_assert_int_eq(gs->state, FALLING);
  ck_assert_int_eq(gs->figure_x, 5);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);

  // Test kLeft: FALLING -> MOVING -> FALLING
  gs = init_game_state();
  info = &gs->game_info;
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  user_input(kStart, false);
  update_current_state();
  gs->current_figure = figure_spawn(info, 4, 0, 0, 0);
  gs->figure_x = 4;
  gs->figure_y = 0;
  gs->state = FALLING;
  user_input(kLeft, false);
  ck_assert_int_eq(gs->state, MOVING);
  ck_assert_int_eq(gs->move_direction, kLeft);
  update_current_state();
  ck_assert_int_eq(gs->state, FALLING);
  ck_assert_int_eq(gs->figure_x, 3);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);

  // Test kDown (hold = false): Single step down
  gs = init_game_state();
  info = &gs->game_info;
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  user_input(kStart, false);
  update_current_state();
  gs->current_figure = figure_spawn(info, 4, 0, 0, 0);
  gs->figure_x = 4;
  gs->figure_y = 0;
  gs->state = FALLING;
  user_input(kDown, false);
  ck_assert_int_eq(gs->state, LOCKING);
  update_current_state();
  ck_assert_int_eq(gs->figure_y, 0);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);

  // Test kDown (hold = true): Drop to bottom
  gs = init_game_state();
  info = &gs->game_info;
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  user_input(kStart, false);
  update_current_state();
  gs->current_figure = figure_spawn(info, 4, 0, 0, 0);
  gs->figure_x = 4;
  gs->figure_y = 0;
  gs->state = FALLING;
  user_input(kDown, true);
  ck_assert_int_eq(gs->state, LOCKING);
  ck_assert_int_eq(gs->current_figure.figure_points[0].point_y, kRow - 1);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);

  // Test kAction: Rotate figure
  gs = init_game_state();
  info = &gs->game_info;
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  user_input(kStart, false);
  update_current_state();
  gs->current_figure =
      figure_spawn(info, 4, 2, 0, 0);  // I-tetromino, moved down for rotation
  gs->figure_x = 4;
  gs->figure_y = 2;
  gs->figure_type = 0;
  gs->rotation_idx = 0;
  gs->state = FALLING;
  user_input(kAction, false);
  ck_assert_int_eq(gs->state, FALLING);
  ck_assert_int_eq(gs->rotation_idx, 1);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);

  // Test ignoring actions when paused
  gs = init_game_state();
  info = &gs->game_info;
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  user_input(kStart, false);
  update_current_state();
  info->pause = 1;
  gs->state = FALLING;
  user_input(kRight, false);
  ck_assert_int_eq(gs->state, FALLING);  // No change
  user_input(kLeft, false);
  ck_assert_int_eq(gs->state, FALLING);
  user_input(kDown, true);
  ck_assert_int_eq(gs->state, FALLING);
  user_input(kAction, false);
  ck_assert_int_eq(gs->state, FALLING);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);

  // Test ignoring actions in START
  gs = init_game_state();
  info = &gs->game_info;
  for (int i = 0; i < kRow; i++)
    for (int j = 0; j < kCol; j++) info->field[i][j] = 0;
  info->pause = 0;
  gs->state = START;
  user_input(kRight, false);
  ck_assert_int_eq(gs->state, START);
  user_input(kLeft, false);
  ck_assert_int_eq(gs->state, START);
  user_input(kDown, true);
  ck_assert_int_eq(gs->state, START);
  user_input(kAction, false);
  ck_assert_int_eq(gs->state, START);
  free_matrix(info->field, kRow);
  free_matrix(info->next, kFigureSize);
}
END_TEST

Suite *tetris_suite(void) {
  Suite *s = suite_create("Tetris");
  TCase *tc_core = tcase_create("Core");
  tcase_add_checked_fixture(tc_core, setup, NULL);
  tcase_add_test(tc_core, test_alloc_matrix);
  tcase_add_test(tc_core, test_free_matrix);
  tcase_add_test(tc_core, test_figure_spawn);
  tcase_add_test(tc_core, test_next_figure_generate);
  tcase_add_test(tc_core, test_next_check);
  tcase_add_test(tc_core, test_start_fsm);
  tcase_add_test(tc_core, test_spawn_fsm);
  tcase_add_test(tc_core, test_falling_fsm);
  tcase_add_test(tc_core, test_moving_fsm);
  tcase_add_test(tc_core, test_rotate_figure);
  tcase_add_test(tc_core, test_clearing_fsm);
  tcase_add_test(tc_core, test_game_over_fsm);
  tcase_add_test(tc_core, test_user_input);
  tcase_add_test(tc_core, test_user_input_movement);
  suite_add_tcase(s, tc_core);
  return s;
}

int main(void) {
  Suite *s = tetris_suite();
  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);
  int nf = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (nf == 0) ? 0 : 1;
}