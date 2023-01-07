CC = clang
CFLAGS = -pedantic -Wall -Wextra -std=c11 -lncurses
CFILES = main.c

ved : $(CFILES)
	$(CC) $(CFLAGS) $(CFILES) -o ved

curses_testkeys : curses-testkeys.c
	$(CC) $(CFLAGS) curses-testkeys.c -o curses_testkeys

.PHONY : run run_creating run_w_existing testkeys

run : ved
	./ved

run_creating : ved
	./ved test

run_w_existing : ved
	cp akatsuki-backup.txt akatsuki.txt
	./ved akatsuki.txt

testkeys : curses_testkeys
	./curses_testkeys
