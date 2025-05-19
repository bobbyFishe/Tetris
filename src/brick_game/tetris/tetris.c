#include "tetris.h"

#ifdef INSTALL
const char* kHighScorePath = "/usr/local/share/tetris/high_score.txt";
#else
const char* kHighScorePath = "brick_game/tetris/high_score.txt";
#endif

const int kTetrominoShapes[][4][kFigureSize][kFigureSize] = {
    {{{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
    {{{0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 1, 1, 1}, {0, 1, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}},
     {{0, 0, 0, 0}, {0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}}},
    {{{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
    {{{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
     {{0, 1, 0, 0}, {1, 1, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}}},
    {{{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 1, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
    {{{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}},
    {{{0, 0, 1, 0}, {0, 0, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 1, 0, 0}, {0, 1, 1, 1}, {0, 0, 0, 0}},
     {{0, 0, 0, 0}, {0, 1, 1, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}},
     {{0, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 1, 0}, {0, 0, 0, 0}}}};

const int* getRotationsPerTetromino() {
  static const int rotations[] = {2, 4, 1, 4, 2, 2, 4};
  return rotations;
}

GameState* getGameState() {
  static GameState gameState = {0};
  return &gameState;
}

int** allocMatrix(int rows, int cols) {
  int** matrix = malloc(rows * sizeof(int*));
  if (!matrix) {
    fprintf(stderr, "Failed to allocate matrix rows\n");
    return NULL;
  }
  for (int i = 0; i < rows; ++i) {
    matrix[i] = calloc(cols, sizeof(int));
    if (!matrix[i]) {
      fprintf(stderr, "Failed to allocate matrix column %d\n", i);
      for (int j = 0; j < i; ++j) {
        free(matrix[j]);
      }
      free(matrix);
      return NULL;
    }
  }
  return matrix;
}

void freeMatrix(int** matrix, int rows) {
  if (matrix) {
    for (int i = 0; i < rows; ++i) {
      free(matrix[i]);
    }
    free(matrix);
  }
}

TetrominoPoints spawnTetromino(GameInfo* gameInfo, int x, int y, int type,
                               int rotationIndex) {
  TetrominoPoints tetromino = {0};
  int count = 0;
  const int(*shape)[kFigureSize] = kTetrominoShapes[type][rotationIndex];
  for (int i = 0; i < kFigureSize && count < kFigurePoints; ++i) {
    for (int j = 0; j < kFigureSize && count < kFigurePoints; ++j) {
      if (shape[i][j]) {
        int newX = j + x;
        int newY = i + y;
        if (newX >= 0 && newX < kCol && newY >= 0 && newY < kRow) {
          gameInfo->field[newY][newX] = shape[i][j];
          tetromino.points[count].x = newX;
          tetromino.points[count].y = newY;
          count++;
        }
      }
    }
  }
  for (; count < kFigurePoints; ++count) {
    tetromino.points[count].x = -1;
    tetromino.points[count].y = -1;
  }
  return tetromino;
}

void generateNextTetromino(GameInfo* gameInfo, int* type, int* rotationIndex) {
  *type = rand() % (sizeof(kTetrominoShapes) / sizeof(kTetrominoShapes[0]));
  *rotationIndex = 0;
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      gameInfo->next[i][j] = kTetrominoShapes[*type][*rotationIndex][i][j];
    }
  }
}

bool hasNextTetromino(const GameInfo* gameInfo) {
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      if (gameInfo->next[i][j]) {
        return true;
      }
    }
  }
  return false;
}

void renderField(GameInfo gameInfo) {
  for (int i = 0; i < kRow; ++i) {
    for (int j = 0; j < kCol; ++j) {
      mvprintw(i, j, gameInfo.field[i][j] ? "o" : " ");
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
      mvprintw(i + 2, j + kCol + 3, gameInfo.next[i][j] ? "o" : " ");
    }
  }
  mvprintw(6, kCol + 3, "Level: %d", gameInfo.level);
  mvprintw(7, kCol + 3, "Score: %d", gameInfo.score);
  mvprintw(8, kCol + 3, "High Score: %d", gameInfo.high_score);
  if (gameInfo.pause == 1) {
    mvprintw(kRow / 2, kCol / 2 - 3, "PAUSED");
  } else if (gameInfo.pause == -1) {
    mvprintw(kRow / 2, kCol / 2 - 5, "GAME OVER");
  }
  refresh();
}

void startGame(GameInfo* gameInfo) {
  gameInfo->field = allocMatrix(kRow, kCol);
  if (!gameInfo->field) {
    fprintf(stderr, "Failed to allocate game field\n");
    return;
  }
  gameInfo->next = allocMatrix(kFigureSize, kFigureSize);
  if (!gameInfo->next) {
    fprintf(stderr, "Failed to allocate next tetromino matrix\n");
    freeMatrix(gameInfo->field, kRow);
    gameInfo->field = NULL;
    return;
  }
  gameInfo->score = 0;
  gameInfo->high_score = 0;
  gameInfo->level = 1;
  gameInfo->speed = kSpeed;
  gameInfo->pause = 0;
  getGameState()->pointsTowardLevel = 0;

  FILE* file = fopen(kHighScorePath, "r");
  if (file) {
    if (fscanf(file, "%d", &gameInfo->high_score) != 1) {
      gameInfo->high_score = 0;
    }
    fclose(file);
  }
}

void spawnTetrominoState(GameInfo* gameInfo, TetrominoPoints* currentTetromino,
                         int* x, int* y, FsmState* state) {
  GameState* gs = getGameState();
  *x = kCol / 2 - kFigureSize / 2;
  *y = 0;

  if (!hasNextTetromino(gameInfo)) {
    generateNextTetromino(gameInfo, &gs->nextTetrominoType, &gs->rotationIndex);
  }

  gs->tetrominoType = gs->nextTetrominoType;
  gs->rotationIndex = 0;

  bool collision = false;
  const int(*shape)[kFigureSize] =
      kTetrominoShapes[gs->tetrominoType][gs->rotationIndex];
  for (int i = 0; i < kFigureSize && !collision; ++i) {
    for (int j = 0; j < kFigureSize && !collision; ++j) {
      if (shape[i][j]) {
        int newX = j + *x;
        int newY = i + *y;
        if (newX >= 0 && newX < kCol && newY >= 0 && newY < kRow) {
          if (gameInfo->field[newY][newX]) {
            *state = kGameOver;
            gameOverState(gameInfo);
            collision = true;
          }
        }
      }
    }
  }
  if (!collision) {
    *currentTetromino =
        spawnTetromino(gameInfo, *x, *y, gs->tetrominoType, gs->rotationIndex);
    generateNextTetromino(gameInfo, &gs->nextTetrominoType, &gs->rotationIndex);
    *state = kFalling;
  }
}

void clearTetromino(GameInfo* gameInfo, TetrominoPoints* currentTetromino) {
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      gameInfo->field[y][x] = 0;
    }
  }
}

void getRotationOffsets(int tetrominoType, int offsets[][2], int* numOffsets) {
  static int defaultOffsets[7][2] = {{0, 0},  {1, 0}, {-1, 0}, {0, 1},
                                     {0, -1}, {2, 0}, {-2, 0}};
  *numOffsets =
      (tetrominoType == 0) ? 7 : 5;  // I-тетромино использует больше смещений
  for (int i = 0; i < *numOffsets; ++i) {
    offsets[i][0] = defaultOffsets[i][0];
    offsets[i][1] = defaultOffsets[i][1];
  }
}

bool isValidRotation(GameInfo* gameInfo, const int shape[][kFigureSize],
                     int newX, int newY) {
  for (int i = 0; i < kFigureSize; ++i) {
    for (int j = 0; j < kFigureSize; ++j) {
      if (shape[i][j]) {
        int checkX = j + newX;
        int checkY = i + newY;
        if (checkX < 0 || checkX >= kCol || checkY < 0 || checkY >= kRow) {
          return false;
        }
        if (gameInfo->field[checkY][checkX]) {
          return false;
        }
      }
    }
  }
  return true;
}

void applyRotation(GameInfo* gameInfo, TetrominoPoints* currentTetromino,
                   int tetrominoType, int nextRotation, int newX, int newY,
                   GameState* gs) {
  gs->rotationIndex = nextRotation;
  gs->tetrominoX = newX;
  gs->tetrominoY = newY;
  *currentTetromino =
      spawnTetromino(gameInfo, newX, newY, tetrominoType, nextRotation);
}

void rotateTetromino(GameInfo* gameInfo, TetrominoPoints* currentTetromino) {
  GameState* gs = getGameState();
  const int* rotations = getRotationsPerTetromino();
  int tetrominoType = gs->tetrominoType;
  int currentRotation = gs->rotationIndex;
  int numRotations = rotations[tetrominoType];

  if (numRotations <= 1) {
    return;
  }

  int nextRotation = (currentRotation + 1) % numRotations;
  const int(*shape)[kFigureSize] =
      kTetrominoShapes[tetrominoType][nextRotation];

  clearTetromino(gameInfo, currentTetromino);
  int offsets[7][2];
  int numOffsets;
  getRotationOffsets(tetrominoType, offsets, &numOffsets);

  bool rotated = false;
  for (int k = 0; k < numOffsets && !rotated; ++k) {
    int newX = gs->tetrominoX + offsets[k][0];
    int newY = gs->tetrominoY + offsets[k][1];
    if (isValidRotation(gameInfo, shape, newX, newY)) {
      applyRotation(gameInfo, currentTetromino, tetrominoType, nextRotation,
                    newX, newY, gs);
      rotated = true;
    }
  }
  if (!rotated) {
    for (int i = 0; i < kFigurePoints; ++i) {
      int x = currentTetromino->points[i].x;
      int y = currentTetromino->points[i].y;
      if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
        gameInfo->field[y][x] = 1;
      }
    }
  }
}

void getLowestPoints(TetrominoPoints* currentTetromino, int lowestY[]) {
  for (int i = 0; i < kCol; ++i) {
    lowestY[i] = -1;
  }
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      if (lowestY[x] == -1 || y > lowestY[x]) {
        lowestY[x] = y;
      }
    }
  }
}

bool canMoveDown(GameInfo* gameInfo, int lowestY[]) {
  for (int x = 0; x < kCol; ++x) {
    if (lowestY[x] != -1) {
      int newY = lowestY[x] + 1;
      if (newY >= kRow ||
          (newY >= 0 && newY < kRow && gameInfo->field[newY][x])) {
        return false;
      }
    }
  }
  return true;
}

void moveTetrominoDown(GameInfo* gameInfo, TetrominoPoints* currentTetromino) {
  // Очистить текущую позицию
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      gameInfo->field[y][x] = 0;
    }
  }
  // Обновить координаты
  for (int i = 0; i < kFigurePoints; ++i) {
    if (currentTetromino->points[i].x >= 0) {
      currentTetromino->points[i].y++;
    }
  }
  // Перерисовать тетромино
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      gameInfo->field[y][x] = 1;
    }
  }
}

void updateTetrominoY(TetrominoPoints* currentTetromino, GameState* gs) {
  int minY = kRow;
  for (int i = 0; i < kFigurePoints; ++i) {
    if (currentTetromino->points[i].y >= 0 &&
        currentTetromino->points[i].y < minY) {
      minY = currentTetromino->points[i].y;
    }
  }
  gs->tetrominoY = minY;
}

void fallingTetrominoState(GameInfo* gameInfo,
                           TetrominoPoints* currentTetromino, FsmState* state) {
  GameState* gs = getGameState();
  int lowestY[kCol];
  getLowestPoints(currentTetromino, lowestY);
  if (canMoveDown(gameInfo, lowestY)) {
    moveTetrominoDown(gameInfo, currentTetromino);
    updateTetrominoY(currentTetromino, gs);
  } else {
    *state = kLocking;
  }
}

void getExtremePoints(TetrominoPoints* currentTetromino, UserAction direction,
                      int extremeX[], int pointsPerY[]) {
  for (int i = 0; i < kRow; ++i) {
    extremeX[i] = (direction == kActionLeft) ? kCol : -1;
    pointsPerY[i] = 0;
  }
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (y >= 0 && y < kRow) {
      pointsPerY[y]++;
      if (direction == kActionLeft) {
        if (x < extremeX[y]) {
          extremeX[y] = x;
        }
      } else {
        if (x > extremeX[y]) {
          extremeX[y] = x;
        }
      }
    }
  }
}

bool canMoveSideways(GameInfo* gameInfo, int extremeX[], int pointsPerY[],
                     int deltaX) {
  for (int y = 0; y < kRow; ++y) {
    if (pointsPerY[y] > 0) {
      int checkX = extremeX[y] + deltaX;
      if (y < 0 || y >= kRow || checkX < 0 || checkX >= kCol ||
          gameInfo->field[y][checkX]) {
        return false;
      }
    }
  }
  return true;
}

void shiftTetromino(GameInfo* gameInfo, TetrominoPoints* currentTetromino,
                    int deltaX) {
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      gameInfo->field[y][x] = 0;
    }
  }
  for (int i = 0; i < kFigurePoints; ++i) {
    if (currentTetromino->points[i].x >= 0) {
      currentTetromino->points[i].x += deltaX;
    }
  }
  for (int i = 0; i < kFigurePoints; ++i) {
    int x = currentTetromino->points[i].x;
    int y = currentTetromino->points[i].y;
    if (x >= 0 && x < kCol && y >= 0 && y < kRow) {
      gameInfo->field[y][x] = 1;
    }
  }
}

void movingTetrominoState(GameInfo* gameInfo, TetrominoPoints* currentTetromino,
                          FsmState* state, int* x, UserAction direction) {
  int deltaX = (direction == kActionLeft) ? -1 : 1;
  int extremeX[kRow];
  int pointsPerY[kRow];

  // Сначала попытаться выполнить боковое смещение
  getExtremePoints(currentTetromino, direction, extremeX, pointsPerY);
  if (canMoveSideways(gameInfo, extremeX, pointsPerY, deltaX)) {
    shiftTetromino(gameInfo, currentTetromino, deltaX);
    *x += deltaX;
  }

  // Затем выполнить один цикл падения
  fallingTetrominoState(gameInfo, currentTetromino, state);

  // Сохранить kFalling, если не kLocking
  if (*state != kLocking) {
    *state = kFalling;
  }
}

void clearLinesState(GameInfo* gameInfo, FsmState* state) {
  GameState* gs = getGameState();
  int linesCleared = 0;
  for (int y = kRow - 1; y >= 0 && linesCleared < 4; --y) {
    bool lineFull = true;
    for (int x = 0; x < kCol; ++x) {
      if (!gameInfo->field[y][x]) {
        lineFull = false;
        break;
      }
    }
    if (lineFull) {
      linesCleared++;
      for (int yy = y; yy > 0; --yy) {
        for (int x = 0; x < kCol; ++x) {
          gameInfo->field[yy][x] = gameInfo->field[yy - 1][x];
        }
      }
      for (int x = 0; x < kCol; ++x) {
        gameInfo->field[0][x] = 0;
      }
      y++;
    }
  }

  if (linesCleared > 0) {
    int points = 0;
    switch (linesCleared) {
      case 1:
        points = kScoreSingleLine;
        break;
      case 2:
        points = kScoreDoubleLine;
        break;
      case 3:
        points = kScoreTripleLine;
        break;
      case 4:
        points = kScoreTetris;
        break;
    }
    gameInfo->score += points;
    gs->pointsTowardLevel += points;

    while (gs->pointsTowardLevel >= kPointsPerLevel &&
           gameInfo->level < kMaxLevel) {
      gameInfo->level++;
      gs->pointsTowardLevel -= kPointsPerLevel;
      gameInfo->speed = kSpeed - (gameInfo->level - 1) * 100;
      if (gameInfo->speed < kMinSpeed) {
        gameInfo->speed = kMinSpeed;
      }
    }

    if (gameInfo->score > gameInfo->high_score) {
      gameInfo->high_score = gameInfo->score;
    }
  }

  *state = kSpawn;
}

void gameOverState(GameInfo* gameInfo) {
  gameInfo->pause = -1;
  if (gameInfo->score > gameInfo->high_score) {
    gameInfo->high_score = gameInfo->score;
  }
  FILE* file = fopen(kHighScorePath, "w");
  if (file) {
    fprintf(file, "%d", gameInfo->high_score);
    fclose(file);
  } else {
    fprintf(stderr, "Failed to write high score to %s\n", kHighScorePath);
  }
}

void userInput(UserAction action, bool hold) {
  GameState* gs = getGameState();
  GameInfo* info = &gs->gameInfo;
  switch (action) {
    case kActionStart:
      if (gs->state == kStart) {
        startGame(info);
        gs->state = kSpawn;
      }
      break;
    case kActionPause:
      if (info->pause == 0) {
        info->pause = 1;
        gs->state = kPaused;
      } else if (info->pause == 1) {
        info->pause = 0;
        gs->state = kFalling;
      }
      break;
    case kActionTerminate:
      gs->state = kGameOver;
      gameOverState(info);
      cleanupGame();
      break;
    case kActionLeft:
    case kActionRight:
      if (!info->pause && gs->state == kFalling) {
        gs->moveDirection = action;
        gs->state = kMoving;
      }
      break;
    case kActionUp:
      break;
    case kActionDown:
      if (!info->pause && (gs->state == kFalling || gs->state == kMoving)) {
        gs->state = kFalling;
        if (hold) {
          fallingTetrominoState(info, &gs->currentTetromino, &gs->state);
        }
        while (gs->state == kFalling) {
          fallingTetrominoState(info, &gs->currentTetromino, &gs->state);
        }
      }
      break;
    case kActionRotate:
      if (!info->pause && gs->state == kFalling) {
        gs->state = kRotating;
        rotateTetromino(info, &gs->currentTetromino);
        gs->state = kFalling;
      }
      break;
  }
}

GameInfo updateCurrentState() {
  GameState* gs = getGameState();
  GameInfo* info = &gs->gameInfo;
  if (!info->pause && gs->state != kGameOver) {
    switch (gs->state) {
      case kSpawn:
        spawnTetrominoState(info, &gs->currentTetromino, &gs->tetrominoX,
                            &gs->tetrominoY, &gs->state);
        break;
      case kFalling:
        fallingTetrominoState(info, &gs->currentTetromino, &gs->state);
        break;
      case kMoving:
        movingTetrominoState(info, &gs->currentTetromino, &gs->state,
                             &gs->tetrominoX, gs->moveDirection);
        break;
      case kLocking:
        gs->state = kClearing;
        break;
      case kClearing:
        clearLinesState(info, &gs->state);
        break;
      default:
        break;
    }
  }
  return *info;
}

void cleanupGame() {
  GameState* gs = getGameState();
  freeMatrix(gs->gameInfo.field, kRow);
  freeMatrix(gs->gameInfo.next, kFigureSize);
  gs->gameInfo.field = NULL;
  gs->gameInfo.next = NULL;
}