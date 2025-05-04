CC = gcc
FLAGS = -Wall -Werror -Wextra

build: tetris.c tetris.h
	> high_score.txt
	$(CC) $(FLAGS) tetris.c -lncurses -o tetris

style:
	@clang-format -style=Google -i *.c *.h