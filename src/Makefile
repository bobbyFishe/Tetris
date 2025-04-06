CC = gcc
FLAGS = -Wall -Werror -Wextra

build: main.c
	$(CC) $(FLAGS) main.c -lncurses

style:
	@clang-format -style=Google -i *.c