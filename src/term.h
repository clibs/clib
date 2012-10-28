
//
// term.h
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#ifndef TERM_H
#define TERM_H

// output

#define term_write(c) printf("\e[" c);

// aliases

#define term_bold term_bright
#define term_clear term_erase

// display

#define term_reset() term_write("0m")
#define term_bright() term_write("1m")
#define term_dim() term_write("2m")
#define term_underline() term_write("4m")
#define term_blink() term_write("5m")
#define term_reverse() term_write("7m")
#define term_hidden() term_write("8m")

// cursor

#define term_hide_cursor() term_write("?25l")
#define term_show_cursor() term_write("?25h")

// size

int
term_size(int *width, int *height);

// movement

void
term_move_by(int x, int y);

void
term_move_to(int x, int y);

// erasing

const char *
term_erase_from_name(const char *name);

int
term_erase(const char *name);

// colors

int
term_color_from_name(const char *name);

int
term_color(const char *name);

int
term_background(const char *name);

#endif /* TERM_H */