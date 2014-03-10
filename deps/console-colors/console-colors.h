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
#ifndef CONSOLE_COLORS_CONSOLE_COLORS_H_
#define CONSOLE_COLORS_CONSOLE_COLORS_H_
#include <stdio.h>

#define CC_COLOR_BITS 5

typedef enum {
    CC_FG_NONE         = 0  << 0,
    CC_FG_BLACK        = 1  << 0,
    CC_FG_DARK_RED     = 2  << 0,
    CC_FG_DARK_GREEN   = 3  << 0,
    CC_FG_DARK_YELLOW  = 4  << 0,
    CC_FG_DARK_BLUE    = 5  << 0,
    CC_FG_DARK_MAGENTA = 6  << 0,
    CC_FG_DARK_CYAN    = 7  << 0,
    CC_FG_GRAY         = 8  << 0,
    CC_FG_DARK_GRAY    = 9  << 0,
    CC_FG_RED          = 10 << 0,
    CC_FG_GREEN        = 11 << 0,
    CC_FG_YELLOW       = 12 << 0,
    CC_FG_BLUE         = 13 << 0,
    CC_FG_MAGENTA      = 14 << 0,
    CC_FG_CYAN         = 15 << 0,
    CC_FG_WHITE        = 16 << 0,

    CC_BG_NONE         = 0  << CC_COLOR_BITS,
    CC_BG_BLACK        = 1  << CC_COLOR_BITS,
    CC_BG_DARK_RED     = 2  << CC_COLOR_BITS,
    CC_BG_DARK_GREEN   = 3  << CC_COLOR_BITS,
    CC_BG_DARK_YELLOW  = 4  << CC_COLOR_BITS,
    CC_BG_DARK_BLUE    = 5  << CC_COLOR_BITS,
    CC_BG_DARK_MAGENTA = 6  << CC_COLOR_BITS,
    CC_BG_DARK_CYAN    = 7  << CC_COLOR_BITS,
    CC_BG_GRAY         = 8  << CC_COLOR_BITS,
    CC_BG_DARK_GRAY    = 9  << CC_COLOR_BITS,
    CC_BG_RED          = 10 << CC_COLOR_BITS,
    CC_BG_GREEN        = 11 << CC_COLOR_BITS,
    CC_BG_YELLOW       = 12 << CC_COLOR_BITS,
    CC_BG_BLUE         = 13 << CC_COLOR_BITS,
    CC_BG_MAGENTA      = 14 << CC_COLOR_BITS,
    CC_BG_CYAN         = 15 << CC_COLOR_BITS,
    CC_BG_WHITE        = 16 << CC_COLOR_BITS
} cc_color_t;

#ifndef COMMON_LVB_LEADING_BYTE
#define COMMON_LVB_LEADING_BYTE    0x0100
#endif

#ifndef COMMON_LVB_TRAILING_BYTE
#define COMMON_LVB_TRAILING_BYTE   0x0200
#endif

#ifndef COMMON_LVB_GRID_HORIZONTAL
#define COMMON_LVB_GRID_HORIZONTAL 0x0400
#endif

#ifndef COMMON_LVB_GRID_LVERTICAL
#define COMMON_LVB_GRID_LVERTICAL  0x0800
#endif

#ifndef COMMON_LVB_GRID_RVERTICAL
#define COMMON_LVB_GRID_RVERTICAL  0x1000
#endif

#ifndef COMMON_LVB_REVERSE_VIDEO
#define COMMON_LVB_REVERSE_VIDEO   0x4000
#endif

#ifndef COMMON_LVB_UNDERSCORE
#define COMMON_LVB_UNDERSCORE      0x8000
#endif

/**
 * @param color {console_color_t} Console color. We can pass (FG | BG) as color.
 * @param stream {FILE*} `stdout` or `stderr`. Others will be passed to fprintf
 * without colors.
 * @param format {const char*} Format string fprintf will take.
 * @return {int} fprintf returned value.
 *
 * CAUTION(Yusuke Suzuki): bright FG & dark BG combination doesn't works
 * correctly on some terminals, but this is an well-known issue.
 */
int cc_fprintf(cc_color_t color, FILE* stream, const char* format, ...);

#endif  /* CONSOLE_COLORS_CONSOLE_COLORS_H_ */
/* vim: set sw=4 ts=4 et tw=80 : */
