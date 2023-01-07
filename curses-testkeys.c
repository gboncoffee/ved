#include<curses.h>
#include<string.h>

int main() {

    initscr();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    curs_set(0);

    for (;;) {
        int c = getch();
        move(LINES / 2, 0);
        clrtoeol();
        const char *str = keyname(c);
        mvaddstr(LINES / 2, (COLS - strlen(str)) / 2 - 1, "'");
        addstr(str);
        addstr("'");
    }

    endwin();
}
