#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<locale.h>
#include<stdbool.h>
#include<assert.h>
#include<curses.h>

/*
 * TODO: actually edit text
 */

#define VERSION "0.0.0-dev"

#define gety() getcury(stdscr)
#define getx() getcurx(stdscr)
#define maxy() (LINES - 2)
#define maxx() (COLS - 1)
#define key(str) (strcmp(k, str) == 0)
#define curline() (buf->lines[buf->fls + gety()])
#define curline_idx() (buf->fls + gety())
#define on_impossible() (getx() > (int) strlen(curline()))
#define normalize_cursor(buf) if (on_impossible()) move_cursor(FarRight, buf)

/*
 * Copyright (c) 2023 Gabriel G. de Brito
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the “Software”), to deal 
 * in the Software without restriction, including without limitation the rights to 
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
 * of the Software, and to permit persons to whom the Software is furnished to do 
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all 
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 */

typedef struct Buffer {
    char **lines;
    int size;
    int fls;
    char *name;
    bool can_discard_name;
    FILE *fp;
} Buffer;

typedef enum Direction {
    Left,
    Right,
    Up,
    Down,
    FarRight,
    FarLeft,
} Direction;

void redraw(Buffer *buf) {
    int lastx = getx();
    int lasty = gety();

    for (int y = 0; y <= maxy(); y++) {
        move(y, 0);
        clrtoeol();
        if (y + buf->fls < buf->size)
            addstr(buf->lines[buf->fls + y]);
        else
            addstr("~");
    }

    move(LINES - 1, 0);

    // revert colors for statusline
    attron(A_REVERSE);
    mvaddstr(LINES - 1, 0, buf->name);
    attroff(A_REVERSE);

    move(lastx, lasty);
}

void move_cursor(Direction dir, Buffer *buf);
void paginate(Direction dir, Buffer *buf) {

    int cury = gety(); int curx = getx();

    switch(dir) {
        case Up:
            if (buf->fls > 0) {
                buf->fls--;
                redraw(buf);
                move(cury, curx);
                normalize_cursor(buf);
            }
            break;
        case Down:
            if (curline_idx() < buf->size - 1) {
                buf->fls++;
                redraw(buf);
                move(cury, curx);
                normalize_cursor(buf);
            }
            break;
        default:
            assert(0 && "Unreachable");
    }
}

void move_cursor(Direction dir, Buffer *buf) {

    switch(dir) {
        case Right:
            if (getx() < (int) strlen(curline()))
                move(gety(), getx() + 1);
            else if (curline_idx() < buf->size - 1) {
                move_cursor(Down, buf);
                move_cursor(FarLeft, buf);
            }
            break;
        case Left:
            if (getx() > 0)
                move(gety(), getx() - 1);
            else if (curline_idx() > 0) {
                move_cursor(Up, buf);
                move_cursor(FarRight, buf);
            }
            break;
        case Up:
            if (gety() > 0) {
                move(gety() - 1, getx());
                normalize_cursor(buf);
            } else {
                paginate(Up, buf);
                move(0, getx());
                normalize_cursor(buf);
            }
            break;
        case Down:
            if (curline_idx() < buf->size - 1) {
                if (gety() < maxy()) {
                    move(gety() + 1, getx());
                    normalize_cursor(buf);
                } else {
                    int lastx = getx();
                    paginate(Down, buf);
                    move(maxy(), lastx);
                    normalize_cursor(buf);
                }
            }
            break;
        case FarRight:
            move(gety(), strlen(curline()));
            break;
        case FarLeft:
            move(gety(), 0);
    }
}

Buffer *new_buf() {
    char **lines = (char**) malloc(sizeof(char*));
    lines[0] = (char*) malloc(sizeof(char));
    lines[0][0] = '\0';

    Buffer *new = (Buffer*) malloc(sizeof(Buffer));
    new->lines = lines;
    new->size = 1;
    new->name = "[No Name]";
    new->can_discard_name = false;
    new->fp = NULL;
    new->fls = 0;

    return new;
}

void discard_buf(Buffer *buf) {
    for (int i = 0; i < buf->size; i++) {
        free(buf->lines[i]);
    }
    if (buf->can_discard_name)
        free(buf->name);
    if (buf->fp != NULL)
        fclose(buf->fp);
    free(buf);
}

Buffer *new_buf_w_file(char *path) {

    Buffer *new;

    FILE *fp = fopen(path, "r+");
    if (fp == NULL) {
        new = new_buf();
        new->fp = fopen(path, "w+");
        if (new->fp == NULL) {
            discard_buf(new);
            return NULL;
        }
        errno = 0;
        new->name = path;
        return new;
    }

    new = (Buffer*) malloc(sizeof(Buffer));
    new->name = path;
    new->fp = fp;

    // get number of lines in stream
    int n_lines = 1;
    for (;;) {
        char c = getc(fp);
        if (c == '\n') {
            n_lines++;
            if ((c = getc(fp)) == EOF) {
                n_lines--;
                break;
            }
            ungetc(c, fp);
        } else if (c == EOF) {
            n_lines++;
            break;
        }
    }

    fseek(fp, 0, SEEK_SET);

    char **lines = (char**) malloc(sizeof(char*) * n_lines);
    new->size = n_lines;

    int cur_line = 0;
    int l_size = 0;
    for (;;) {

        char c = getc(fp);

        if (c == '\n' || c == EOF) {
            if (l_size == 0 && c == EOF)
                break;

            if (l_size > 0) {
                fseek(fp, -(l_size + 1), SEEK_CUR);
                lines[cur_line] = (char*) malloc(sizeof(char) * (l_size + 1));
                fgets(lines[cur_line], l_size + 1, fp);
                // advance over the \n
                getc(fp);
            } else {
                lines[cur_line] = (char*) malloc(sizeof(char));
                lines[cur_line][0] = '\0';
            }

            l_size = 0;
            cur_line++;
            if (c == EOF)
                break;
            continue;
        }

        l_size++;
    }

    new->lines = lines;
    new->fls = 0;

    return new;
}

void insert(Buffer *buf, int line, int pos, char c) {

    char *cur = buf->lines[line];
    int orig_len = strlen(cur);

    char *newline = (char*) malloc(sizeof(char) * (orig_len + 2));
    if (pos != 0)
        memcpy(newline, cur, pos);
    newline[pos] = c;
    if (pos != orig_len)
        memcpy(newline + pos + 1, cur + pos, orig_len - pos);

    newline[orig_len + 1] = '\0';

    free(buf->lines[line]);
    buf->lines[line] = newline;
}

void insert_line(Buffer *buf, int line, char *text) {
    buf->lines = realloc(buf->lines, sizeof(char*) * (buf->size + 1));
    // shift everything after the line to the right
    for (int i = buf->size; i > line; i--)
        buf->lines[i] = buf->lines[i - 1];
    buf->lines[line] = text;
    buf->size++;
}

void split_line(Buffer *buf, int line, int pos) {
    char *cur = buf->lines[line];
    int orig_len = strlen(cur);
    char *f_half = (char*) malloc(sizeof(char) * (pos + 1));
    char *s_half = (char*) malloc(sizeof(char) * (strlen(cur) - pos));

    int i;
    for (i = 0; i < pos; i++)
        f_half[i] = cur[i];
    f_half[i++] = '\0';
    for (i = 0; i + pos < orig_len; i++)
        s_half[i] = cur[pos + i];
    s_half[i++] = '\0';

    free(cur);
    buf->lines[line] = f_half;
    insert_line(buf, line + 1, s_half);
}

void delete(Buffer *buf, int line, int pos) {
    char *cur = buf->lines[line];
    int orig_len = strlen(cur);

    char *newline = (char*) malloc(sizeof(char) * (orig_len));

    if (pos < 0 || pos >= orig_len)
        assert(0 && "TODO! Remove/merge lines");

    memcpy(newline, cur, pos);
    memcpy(newline + pos, cur + pos + 1, orig_len - pos);

    free(buf->lines[line]);
    buf->lines[line] = newline;
}

int main_loop(Buffer *buf) {

    const char *k;
    for (;;) {
        k = keyname(getch());

        if (key("^X")) {
            break;

        } else if (key("KEY_RESIZE")) {
            redraw(buf);

        } else if (key("KEY_LEFT") || key("^B")) {
            move_cursor(Left, buf);

        } else if (key("KEY_RIGHT") || key("^F")) {
            move_cursor(Right, buf);

        } else if (key("KEY_UP") || key("^P")) {
            move_cursor(Up, buf);

        } else if (key("KEY_DOWN") || key("^N")) {
            move_cursor(Down, buf);

        } else if (key("KEY_END") || key("^E")) {
            move_cursor(FarRight, buf);

        } else if (key("KEY_HOME") || key("^A")) {
            move_cursor(FarLeft, buf);

        } else if (key("KEY_PPAGE")) {
            paginate(Up, buf);
            normalize_cursor(buf);

        } else if (key("KEY_NPAGE")) {
            paginate(Down, buf);
            normalize_cursor(buf);

        } else if (key("KEY_BACKSPACE")) {
            int lastx = getx(); int lasty = gety();
            delete(buf, curline_idx(), getx() - 1);
            redraw(buf);
            move(lasty, lastx - 1);
            normalize_cursor(buf);

        } else if (key("KEY_DC") || key("^D")) {
            int lastx = getx(); int lasty = gety();
            delete(buf, curline_idx(), getx());
            redraw(buf);
            move(lasty, lastx);
            normalize_cursor(buf);

        } else if (key("^M")) {
            int lasty = gety();
            split_line(buf, curline_idx(), getx());
            redraw(buf);
            if (lasty + 1 <= maxy())
                move(lasty + 1, 0);
            else {
                move(lasty, 0);
                paginate(Down, buf);
            }

        } else if (strlen(k) == 1) {
            int lastx = getx(); int lasty = gety();
            insert(buf, curline_idx(), getx(), k[0]);
            redraw(buf);
            move(lasty, lastx + 1);
        }
    }

    return errno;
}

#define LINE1 "VED - Visual EDitor -- version "VERSION
#define LINE2 "VED is free and open source software licensed under the MIT license"
#define LINE3 "Help the development of VED: https://github.com/gboncoffee/ved"
#define LINE4 "Copyright (c) 2023 Gabriel G. de Brito"
#define ADD_MSG(msg, line) move(line, ((maxx() + 1) - sizeof(msg)) / 2); addstr(msg)

void draw_welcome_msg() {

    // we need to literally visually see which is the biggest line in the case
    // of changing the message
    if (sizeof(LINE2) > (unsigned long) maxx() - 1) return;

    ADD_MSG(LINE1, maxy() / 2 - 3);
    ADD_MSG(LINE2, maxy() / 2 - 2);
    ADD_MSG(LINE3, maxy() / 2);
    ADD_MSG(LINE4, maxy() / 2 + 2);

    move(0, 0);
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "POSIX");

    // init ncurses
    initscr();
    start_color();
    use_default_colors();
    cbreak();
    noecho();
    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);

    Buffer *buf;

    if (argc > 1) {
        buf = new_buf();
        buf = new_buf_w_file(argv[1]);
        buf->can_discard_name = false;
        redraw(buf);
    } else {
        buf = new_buf();
        buf->can_discard_name = false;
        redraw(buf);
        draw_welcome_msg();
    }

    main_loop(buf);

    endwin();

    return errno;
}
