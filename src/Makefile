CC = gcc
CFLAGS = -Wall -Werror -Wextra -Ibrick_game/tetris
LIBS = -lncurses
PATH_BACK = brick_game/tetris
PATH_FRONT = gui/cli
PROGRAM = tetris

SOURCES = $(PATH_BACK)/tetris.c $(PATH_FRONT)/main.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = $(PATH_BACK)/tetris.h


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(PROGRAM)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

style:
	clang-format -style=Google -i $(SOURCES) $(HEADERS)

install: $(SOURCES) $(HEADERS) 
	$(CC) $(CFLAGS) -DINSTALL $(SOURCES) $(LIBS) -o $(PROGRAM)
	install -m 755 $(PROGRAM) /usr/local/bin/$(PROGRAM)
	mkdir -p /usr/local/share/$(PROGRAM)
	[ -f $(PATH_BACK)/high_score.txt ] || echo "0" > /usr/local/share/$(PROGRAM)/high_score.txt
	[ -f $(PATH_BACK)/high_score.txt ] && install -m 644 $(PATH_BACK)/high_score.txt /usr/local/share/$(PROGRAM)/high_score.txt || true

uninstall:
	rm -f /usr/local/bin/$(PROGRAM)
	rm -f /usr/local/share/$(PROGRAM)/high_score.txt
	rmdir /usr/local/share/$(PROGRAM) || true
clean:
	rm -f $(PROGRAM) $(OBJECTS)