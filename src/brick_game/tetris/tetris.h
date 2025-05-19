#ifndef TETRIS_TETRIS_H_
#define TETRIS_TETRIS_H_

#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Constants for game field dimensions and settings.
enum {
  kRow = 20,               // Number of rows in the game field.
  kCol = 10,               // Number of columns in the game field.
  kFigureSize = 4,         // Size of tetromino matrix.
  kFigurePoints = 4,       // Number of points in a tetromino.
  kSpeed = 1000,           // Initial game speed (ms).
  kPointsPerLevel = 600,   // Points required to level up.
  kScoreSingleLine = 100,  // Points for clearing one line.
  kScoreDoubleLine = 300,  // Points for clearing two lines.
  kScoreTripleLine = 700,  // Points for clearing three lines.
  kScoreTetris = 1500,     // Points for clearing four lines.
  kMaxLevel = 10,          // Maximum level.
  kMinSpeed = 100          // Minimum speed (ms).
};

// Path to the high score file (defined in tetris.c).
extern const char* kHighScorePath;

// States of the game finite state machine.
typedef enum {
  kStart,     // Game start.
  kSpawn,     // Spawning a new tetromino.
  kFalling,   // Tetromino falling.
  kMoving,    // Tetromino moving left/right.
  kRotating,  // Tetromino rotating.
  kLocking,   // Tetromino locking in place.
  kClearing,  // Clearing completed lines.
  kPaused,    // Game paused.
  kGameOver   // Game over.
} FsmState;

// User input actions.
typedef enum {
  kActionStart,      // Start the game.
  kActionPause,      // Pause the game.
  kActionTerminate,  // Terminate the game.
  kActionLeft,       // Move left.
  kActionRight,      // Move right.
  kActionUp,         // Unused.
  kActionDown,       // Accelerate falling.
  kActionRotate      // Rotate tetromino.
} UserAction;

// Represents a single point of a tetromino.
typedef struct {
  int x;  // X-coordinate.
  int y;  // Y-coordinate.
} Point;

// Stores the points of the current tetromino.
typedef struct {
  Point points[kFigurePoints];  // Array of points.
} TetrominoPoints;

// Game state information for rendering.
typedef struct {
  int** field;     // Game field.
  int** next;      // Next tetromino.
  int score;       // Current score.
  int high_score;  // High score.
  int level;       // Current level.
  int speed;       // Game speed (ms).
  bool pause;      // Pause flag.
} GameInfo;

// Internal game state.
typedef struct {
  FsmState state;            // Current state of the finite state machine.
  int tetrominoX;            // X-coordinate of the tetromino.
  int tetrominoY;            // Y-coordinate of the tetromino.
  int tetrominoType;         // Type of the current tetromino.
  int nextTetrominoType;     // Type of the next tetromino.
  int rotationIndex;         // Rotation index.
  UserAction moveDirection;  // Movement direction.
  TetrominoPoints currentTetromino;  // Current tetromino.
  GameInfo gameInfo;                 // Game information.
  int pointsTowardLevel;             // Points toward the next level.
} GameState;

// Tetromino shapes with rotations (I, L, O, T, S, Z, J).
extern const int kTetrominoShapes[][4][kFigureSize][kFigureSize];

/**
 * Initializes and runs the Tetris game.
 * @return 0 on successful termination, non-zero on error.
 */
int runTetris();

/**
 * Processes user input to control the game.
 * @param action The user action (e.g., move left, pause).
 * @param hold Whether the action is held (for continuous movement).
 */
void userInput(UserAction action, bool hold);

/**
 * Updates the game state and returns the current state for rendering.
 * @return The current game state.
 */
GameInfo updateCurrentState();

/**
 * Returns the number of rotations per tetromino type (I, L, O, T, S, Z, J).
 * @return Pointer to an array of rotation counts.
 */
const int* getRotationsPerTetromino();

/**
 * Returns the singleton game state. Not thread-safe.
 * @return Pointer to the global GameState instance.
 */
GameState* getGameState();

/**
 * Allocates a matrix of given dimensions.
 * @param rows Number of rows.
 * @param cols Number of columns.
 * @return Pointer to the allocated matrix, or NULL on failure.
 */
int** allocMatrix(int rows, int cols);

/**
 * Frees a matrix.
 * @param matrix The matrix to free.
 * @param rows Number of rows.
 */
void freeMatrix(int** matrix, int rows);

/**
 * Spawns a new tetromino on the game field.
 * @param gameInfo Pointer to the game information structure.
 * @param x X-coordinate of the tetromino’s top-left corner.
 * @param y Y-coordinate of the tetromino’s top-left corner.
 * @param type Type of tetromino (0–6 for I, L, O, T, S, Z, J).
 * @param rotationIndex Rotation index of the tetromino.
 * @return TetrominoPoints structure with the tetromino’s coordinates.
 */
TetrominoPoints spawnTetromino(GameInfo* gameInfo, int x, int y, int type,
                               int rotationIndex);

/**
 * Generates a new tetromino for the next slot.
 * @param gameInfo Pointer to the game information structure.
 * @param type Pointer to store the tetromino type.
 * @param rotationIndex Pointer to store the rotation index.
 */
void generateNextTetromino(GameInfo* gameInfo, int* type, int* rotationIndex);

/**
 * Checks if the next tetromino exists.
 * @param gameInfo Pointer to the game information structure.
 * @return True if the next tetromino exists, false otherwise.
 */
bool hasNextTetromino(const GameInfo* gameInfo);

/**
 * Renders the game field and UI using ncurses.
 * @param gameInfo The game information structure.
 */
void renderField(GameInfo gameInfo);

/**
 * Initializes the game state.
 * @param gameInfo Pointer to the game information structure.
 */
void startGame(GameInfo* gameInfo);

/**
 * Handles the spawning of a new tetromino.
 * @param gameInfo Pointer to the game information structure.
 * @param currentTetromino Pointer to the current tetromino’s points.
 * @param x Pointer to the tetromino’s x-coordinate.
 * @param y Pointer to the tetromino’s y-coordinate.
 * @param state Pointer to the game state.
 */
void spawnTetrominoState(GameInfo* gameInfo, TetrominoPoints* currentTetromino,
                         int* x, int* y, FsmState* state);

/**
 * Rotates the current tetromino clockwise.
 * @param gameInfo Pointer to the game information structure.
 * @param currentTetromino Pointer to the current tetromino’s points.
 */
void rotateTetromino(GameInfo* gameInfo, TetrominoPoints* currentTetromino);

/**
 * Handles the falling of the current tetromino.
 * @param gameInfo Pointer to the game information structure.
 * @param currentTetromino Pointer to the current tetromino’s points.
 * @param state Pointer to the game state.
 */
void fallingTetrominoState(GameInfo* gameInfo,
                           TetrominoPoints* currentTetromino, FsmState* state);

/**
 * Handles the moving of the current tetromino left or right.
 * @param gameInfo Pointer to the game information structure.
 * @param currentTetromino Pointer to the current tetromino’s points.
 * @param state Pointer to the game state.
 * @param x Pointer to the tetromino’s x-coordinate.
 * @param direction The movement direction (left or right).
 */
void movingTetrominoState(GameInfo* gameInfo, TetrominoPoints* currentTetromino,
                          FsmState* state, int* x, UserAction direction);

/**
 * Handles the clearing of completed lines.
 * @param gameInfo Pointer to the game information structure.
 * @param state Pointer to the game state.
 */
void clearLinesState(GameInfo* gameInfo, FsmState* state);

/**
 * Sets the game to the game-over state.
 * @param gameInfo Pointer to the game information structure.
 */
void gameOverState(GameInfo* gameInfo);

/**
 * Cleans up game resources.
 */
void cleanupGame();

#endif  // TETRIS_TETRIS_H_