# Tetris

## Overview

This is an implementation of the classic Tetris game in C (C11 standard) using the **ncurses** library for the terminal interface. The program consists of two parts:

* Game logic library (**src/brick_game/tetris**).
* User interface (**src/gui/cli**).

The game logic is implemented using a Finite State Machine (FSM). The library provides functions to handle user input and return the current state of the game field.

## Installation

1. Ensure **gcc**, **make**, and **ncurses-dev** are installed:
   ```bash
   sudo apt-get install build-essential libncurses5-dev libncursesw5-dev
   ```
2. Compile and install:
   ```bash
   make
   sudo make install
   ```
3. Run the game:
   ```bash
   tetris
   ```

## Usage

* **q** : Quit the game.
* **p** : Pause/resume.
* **Enter** : Start the game.
* **Space** : Rotate the figure.
* **Arrow keys** : Move the figure (left, right, down).

High scores are saved in **/usr/local/share/tetris/high_score.txt**.

## Project Structure

* **src/brick_game/tetris**: Game logic (**tetris.c**, **tetris.h**).
* **src/gui/cli**: Interface (**main.c**).
* **Makefile**: Build, install, uninstall, clean.

## Requirements

* Compiler: **gcc** (C11).
* Library: **ncurses**.
* OS: Linux/Unix.

## Development

The program meets the assignment requirements:

* Finite State Machine for logic.
* Modular structure (library + interface).
* High score persistence via file.

## Finite State Machine Diagram

The game logic is managed by a Finite State Machine (FSM) with the following states and transitions:



stateDiagram-v2
    [*] --> START
    START --> SPAWN : kStart
    START --> GAME_OVER : kTerminate

    SPAWN --> FALLING : Successful spawn
    SPAWN --> GAME_OVER : Collision at spawn

    FALLING --> FALLING : Can continue falling
    FALLING --> LOCKING : Collision (bottom/field)
    FALLING --> MOVING : kLeft/kRight
    FALLING --> ROTATING : kAction
    FALLING --> PAUSED : kPause
    FALLING --> LOCKING : kDown (held)
    FALLING --> GAME_OVER : kTerminate

    MOVING --> FALLING : Movement completed
    MOVING --> PAUSED : kPause
    MOVING --> GAME_OVER : kTerminate

    ROTATING --> FALLING : Rotation completed
    ROTATING --> PAUSED : kPause
    ROTATING --> GAME_OVER : kTerminate

    LOCKING --> CLEARING : Figure locked

    CLEARING --> SPAWN : Lines cleared

    PAUSED --> FALLING : kPause
    PAUSED --> GAME_OVER : kTerminate

    GAME_OVER --> [*]
