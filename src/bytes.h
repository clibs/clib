
//
// bytes.h
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#ifndef BYTES
#define BYTES

// max buffer length

#ifndef BYTES_MAX
#define BYTES_MAX 256
#endif

// prototypes

long long
string_to_bytes(const char *str);

char *
bytes_to_string(long long bytes);

#endif