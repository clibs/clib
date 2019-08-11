// Copyright (c) 2014 Eddy Luten - https://github.com/EddyLuten/lib-semver
// Distributed under MIT license.
#ifndef LIB_SEMVER_H
#define LIB_SEMVER_H
#include <stdint.h>   // for uint64_t support
#include <stdio.h>    // for FILE struct
#include <stdbool.h>  // for Booleans

#define LIB_SEMVER_VERSION "0.0.3-alpha"

#if !defined(__cplusplus) && __STDC_VERSION__ < 199901L
#  error Please use a compiler that supports C99
#endif

typedef struct _SemVer
{
  uint64_t major;
  uint64_t minor;
  uint64_t patch;
  uint64_t pre_release_elements;
  char** pre_release;
  char* build_info;
} SemVer;

typedef void* (*semver_malloc_fnc)(size_t);
typedef void* (*semver_realloc_fnc)(void*,size_t);
typedef void (*semver_free_fnc)(void*);

void semver_config(semver_malloc_fnc custom_malloc,
                   semver_realloc_fnc custom_realloc,
                   semver_free_fnc custom_free,
                   FILE* debug_output);
SemVer* semver_parse(const char* string);
SemVer* semver_create(uint64_t major, uint64_t minor, uint64_t patch);
bool semver_add_pre_release_component(SemVer** version, const char* component);
bool semver_add_build_info_component(SemVer** version, const char* component);
void semver_destroy(SemVer* version);
char* semver_to_string(SemVer* version);
int semver_compare(SemVer* a, SemVer* b);
int semver_is_stable(SemVer* version);

#endif
