CC = gcc
CFLAGS = -Wall -Werror -Wextra -Ibrick_game/tetris -std=c11 -g
TEST_CFLAGS = $(CFLAGS) -fprofile-arcs -ftest-coverage
LIBS = -lncurses
TEST_LIBS = -lcheck -lm -lgcov -lsubunit
PATH_BACK = brick_game/tetris
PATH_FRONT = gui/cli
PATH_TEST = tests
PROGRAM = tetris
VERSION = 1.0
TEST = test_tetris

SOURCES = $(PATH_BACK)/tetris.c $(PATH_FRONT)/main.c
OBJECTS = $(SOURCES:.c=.o)
TEST_SOURCES = $(PATH_TEST)/test_tetris.c
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)
HEADERS = $(PATH_BACK)/tetris.h


all: $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(PROGRAM)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

style:
	clang-format -style=Google -i $(SOURCES) $(HEADERS) $(TEST_SOURCES)

install: $(SOURCES) $(HEADERS) 
	$(CC) $(CFLAGS) -DINSTALL $(SOURCES) $(LIBS) -o $(PROGRAM)
	install -m 755 $(PROGRAM) /usr/local/bin/$(PROGRAM)
	mkdir -p /usr/local/share/$(PROGRAM)
	[ -f $(PATH_BACK)/high_score.txt ] || (echo "0" > /usr/local/share/$(PROGRAM)/high_score.txt && chmod 666 /usr/local/share/$(PROGRAM)/high_score.txt)
	[ -f $(PATH_BACK)/high_score.txt ] && install -m 666 $(PATH_BACK)/high_score.txt /usr/local/share/$(PROGRAM)/high_score.txt || true

uninstall:
	rm -f /usr/local/bin/$(PROGRAM)
	rm -f /usr/local/share/$(PROGRAM)/high_score.txt
	rmdir /usr/local/share/$(PROGRAM) || true

dvi:
	pandoc README.md -o documentation.pdf

dist:
	mkdir -p tetris-$(VERSION)
	cp -r brick_game gui Makefile README.md tetris-$(VERSION)/
	tar -czf tetris-$(VERSION).tar.gz tetris-$(VERSION)
	rm -rf tetris-$(VERSION)

test: $(TEST)
	rm -f *.gcda
	./$(TEST)

$(TEST): $(TEST_OBJECTS) $(PATH_BACK)/tetris_test.o 
	$(CC) $(TEST_OBJECTS) $(PATH_BACK)/tetris_test.o $(LIBS) $(TEST_LIBS) -o $(TEST)

$(PATH_TEST)/%.o: $(PATH_TEST)/%.c $(HEADERS)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(PATH_BACK)/tetris_test.o: $(PATH_BACK)/tetris.c $(HEADERS)
	$(CC) $(TEST_CFLAGS) -c $< -o $@

gcov_report: test
	lcov --capture --directory . --output-file coverage.info
	lcov --extract coverage.info '*brick_game/tetris/tetris.c' -o coverage_tetris.info
	lcov --list coverage_tetris.info
	genhtml coverage_tetris.info --output-directory coverage_report
	xdg-open coverage_report/index.html
	rm -f coverage.info coverage_tetris.info

valgrind_tetris: $(PROGRAM)
	echo "q" | valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind_tetris_report.txt --error-exitcode=1 --suppressions=$(PATH_BACK)/ncurses.supp ./$(PROGRAM)
	@echo "Valgrind report for tetris saved to valgrind_tetris_report.txt"

clean:
	rm -f $(PROGRAM) $(OBJECTS) brick_game/tetris/high_score.txt documentation.pdf tetris-1.0.tar.gz
	rm -f $(PATH_TEST)/*.gcno $(PATH_TEST)/*.gcda $(PATH_TEST)/*.gcov $(PATH_TEST)/*.o *.info test_tetris
	rm -f $(PATH_BACK)/*.gcno $(PATH_BACK)/*.gcda $(PATH_BACK)/*.gcov $(PATH_BACK)/*.o
	rm -f valgrind_tetris_report.txt
	rm -rf coverage_report
