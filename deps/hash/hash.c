
//
// hash.c
//
// Copyright (c) 2012 TJ Holowaychuk <tj@vision-media.ca>
//

#include "hash.h"

/*
 * Set hash `key` to `val`.
 */

inline void
hash_set(hash_t *self, char *key, void *val) {
  int ret;
  khiter_t k = kh_put(ptr, self, key, &ret);
  kh_value(self, k) = val;
}

/*
 * Get hash `key`, or NULL.
 */

inline void *
hash_get(hash_t *self, char *key) {
  khiter_t k = kh_get(ptr, self, key);
  return k == kh_end(self) ? NULL : kh_value(self, k);
}

/*
 * Check if hash `key` exists.
 */

inline int
hash_has(hash_t *self, char *key) {
  khiter_t k = kh_get(ptr, self, key);
  return kh_exist(self, k);
}

/*
 * Remove hash `key`.
 */

void
hash_del(hash_t *self, char *key) {
  khiter_t k = kh_get(ptr, self, key);
  kh_del(ptr, self, k);
}

// tests

#ifdef TEST_HASH

#include <stdio.h>
#include <assert.h>
#include <string.h>

void
test_hash_set() {
  hash_t *hash = hash_new();
  assert(0 == hash_size(hash));

  hash_set(hash, "name", "tobi");
  hash_set(hash, "species", "ferret");
  assert(2 == hash_size(hash));

  assert(0 == strcmp("tobi", hash_get(hash, "name")));
  assert(0 == strcmp("ferret", hash_get(hash, "species")));
}

void
test_hash_get() {
  hash_t *hash = hash_new();
  hash_set(hash, "foo", "bar");
  assert(0 == strcmp("bar", hash_get(hash, "foo")));
  assert(NULL == hash_get(hash, "bar"));
}

void
test_hash_has() {
  hash_t *hash = hash_new();
  hash_set(hash, "foo", "bar");
  assert(1 == hash_has(hash, "foo"));
  assert(0 == hash_has(hash, "bar"));
}

void
test_hash_size() {
  hash_t *hash = hash_new();
  assert(0 == hash_size(hash));
  hash_set(hash, "foo", "bar");
  assert(1 == hash_size(hash));
  hash_set(hash, "bar", "baz");
  assert(2 == hash_size(hash));
}

void
test_hash_del() {
  hash_t *hash = hash_new();
  hash_set(hash, "foo", "bar");
  assert(1 == hash_has(hash, "foo"));
  assert(0 == hash_has(hash, "bar"));
  hash_del(hash, "foo");
  hash_del(hash, "bar");
  assert(0 == hash_has(hash, "foo"));
}

void
test_hash_clear() {
  hash_t *hash = hash_new();
  hash_set(hash, "foo", "bar");
  hash_set(hash, "bar", "baz");
  hash_set(hash, "raz", "jaz");
  assert(3 == hash_size(hash));
  hash_clear(hash);
  assert(0 == hash_size(hash));
}

void
test_hash_each() {
  hash_t *hash = hash_new();
  hash_set(hash, "name", "tj");
  hash_set(hash, "age", "25");

  const char *keys[2];
  void *vals[2];
  int n = 0;

  hash_each(hash, {
    keys[n] = key;
    vals[n] = val;
    n++;
  });

  assert(0 == strcmp("age", keys[0]) || 0 == strcmp("name", keys[0]));
  assert(0 == strcmp("age", keys[1]) || 0 == strcmp("name", keys[1]));
  assert(0 == strcmp("25", vals[0]) || 0 == strcmp("tj", vals[0]));
  assert(0 == strcmp("25", vals[1]) || 0 == strcmp("tj", vals[1]));
}

void
test_hash_each_key() {
  hash_t *hash = hash_new();
  hash_set(hash, "name", "tj");
  hash_set(hash, "age", "25");

  const char *keys[2];
  int n = 0;

  hash_each_key(hash, {
    keys[n++] = key;
  });

  assert(0 == strcmp("age", keys[0]) || 0 == strcmp("name", keys[0]));
  assert(0 == strcmp("age", keys[1]) || 0 == strcmp("name", keys[1]));
}

void
test_hash_each_val() {
  hash_t *hash = hash_new();
  hash_set(hash, "name", "tj");
  hash_set(hash, "age", "25");

  void *vals[2];
  int n = 0;

  hash_each_val(hash, {
    vals[n++] = val;
  });

  assert(0 == strcmp("25", vals[0]) || 0 == strcmp("tj", vals[0]));
  assert(0 == strcmp("25", vals[1]) || 0 == strcmp("tj", vals[1]));
}

int
main(){
  test_hash_set();
  test_hash_get();
  test_hash_has();
  test_hash_del();
  test_hash_size();
  test_hash_clear();
  test_hash_each();
  test_hash_each_key();
  test_hash_each_val();
  printf("\n  \e[32m\u2713 \e[90mok\e[0m\n\n");
  return 0;
}

#endif