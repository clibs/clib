/*
  Copyright (C) 2014 Yusuke Suzuki <utatane.tea@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "console-colors.h"

static int Write(FILE *stream, const char *format, va_list ap) {
#if defined(_WIN32) && !defined(__MINGW32__)
    return vfprintf_s(stream, format, ap);
#else
    return vfprintf(stream, format, ap);
#endif
}

#ifdef _WIN32
static const WORD FG[] = {
/* NONE         */  0,
/* BLACK        */  0,
/* DARK_RED     */  FOREGROUND_RED,
/* DARK_GREEN   */  FOREGROUND_GREEN,
/* DARK_YELLOW  */  FOREGROUND_RED | FOREGROUND_GREEN,
/* DARK_BLUE    */  FOREGROUND_BLUE,
/* DARK_MAGENTA */  FOREGROUND_RED | FOREGROUND_BLUE,
/* DARK_CYAN    */  FOREGROUND_GREEN | FOREGROUND_BLUE,
/* GRAY         */  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED,
/* DARK_GRAY    */  FOREGROUND_INTENSITY,
/* RED          */  FOREGROUND_INTENSITY | FOREGROUND_RED,
/* GREEN        */  FOREGROUND_INTENSITY | FOREGROUND_GREEN,
/* YELLOW       */  FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
/* BLUE         */  FOREGROUND_INTENSITY | FOREGROUND_BLUE,
/* MAGENTA      */  FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
/* CYAN         */  FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
/* WHITE        */  FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED,
};

static const WORD BG[] = {
/* NONE         */  0,
/* BLACK        */  0,
/* DARK_RED     */  BACKGROUND_RED,
/* DARK_GREEN   */  BACKGROUND_GREEN,
/* DARK_YELLOW  */  BACKGROUND_RED | BACKGROUND_GREEN,
/* DARK_BLUE    */  BACKGROUND_BLUE,
/* DARK_MAGENTA */  BACKGROUND_RED | BACKGROUND_BLUE,
/* DARK_CYAN    */  BACKGROUND_GREEN | BACKGROUND_BLUE,
/* GRAY         */  BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED,
/* DARK_GRAY    */  BACKGROUND_INTENSITY,
/* RED          */  BACKGROUND_INTENSITY | BACKGROUND_RED,
/* GREEN        */  BACKGROUND_INTENSITY | BACKGROUND_GREEN,
/* YELLOW       */  BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN,
/* BLUE         */  BACKGROUND_INTENSITY | BACKGROUND_BLUE,
/* MAGENTA      */  BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE,
/* CYAN         */  BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE,
/* WHITE        */  BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED,
};

static WORD ExtractForeground(WORD attributes) {
    const WORD mask =
        FOREGROUND_INTENSITY |
        FOREGROUND_GREEN |
        FOREGROUND_BLUE |
        FOREGROUND_RED;
    return attributes & mask;
}

static WORD ExtractBackground(WORD attributes) {
    const WORD mask =
        BACKGROUND_INTENSITY |
        BACKGROUND_GREEN |
        BACKGROUND_BLUE |
        BACKGROUND_RED;
    return attributes & mask;
}

static WORD ExtractOthers(WORD attributes) {
    const WORD mask =
        COMMON_LVB_LEADING_BYTE |
        COMMON_LVB_TRAILING_BYTE |
        COMMON_LVB_GRID_HORIZONTAL |
        COMMON_LVB_GRID_LVERTICAL |
        COMMON_LVB_GRID_RVERTICAL |
        COMMON_LVB_REVERSE_VIDEO |
        COMMON_LVB_UNDERSCORE;
    return attributes & mask;
}

static WORD Generate(unsigned int fg, unsigned int bg, WORD attributes) {
    WORD result = ExtractOthers(attributes);
    result |= ((fg == 0) ? ExtractForeground(attributes) : FG[fg]);
    result |= ((bg == 0) ? ExtractBackground(attributes) : BG[bg]);
    return result;
}

#else

static inline unsigned int Shift(
        unsigned int val, unsigned int normal, unsigned int bright) {
    if (val == 0) {
        return 9 + normal;
    }
    val -= 1;
    /* background */
    if (val >= 8) {
        return (val - 8) + bright;
    }
    return val + normal;
}

static void UnixTerminalColorize(
        FILE* stream, unsigned int fg, unsigned int bg) {
    fprintf(stream, "\x1B[39;49;%u;%um", Shift(fg, 30, 90), Shift(bg, 40, 100));
}

static void UnixTerminalRestore(FILE* stream) {
    fputs("\x1B[39;49m", stream);
}
#endif  /* _WIN32 */

int cc_fprintf(cc_color_t color, FILE* stream, const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    int result = -EINVAL;

    if (stream != stdout && stream != stderr) {
        result = Write(stream, format, ap);
        goto finish;
    }

    const unsigned int fg =
        color & ((1 << CC_COLOR_BITS) - 1);
    const unsigned int bg =
        (color >> CC_COLOR_BITS) & ((1 << CC_COLOR_BITS) - 1);

#ifdef _WIN32
    const HANDLE console = GetStdHandle(
            stream == stdout ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    if (console == INVALID_HANDLE_VALUE) {
        result = Write(stream, format, ap);
        goto finish;
    }
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(console, &csbi)) {
        result = Write(stream, format, ap);
        goto finish;
    }
    SetConsoleTextAttribute(console, Generate(fg, bg, csbi.wAttributes));
    result = Write(stream, format, ap);
    SetConsoleTextAttribute(console, csbi.wAttributes);
#else
    UnixTerminalColorize(stream, fg, bg);
    Write(stream, format, ap);
    UnixTerminalRestore(stream);
#endif  /* _WIN32 */

finish:
    va_end(ap);
    return result;
}

/* vim: set sw=4 ts=4 et tw=80 : */
