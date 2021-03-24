#ifndef CLIB_SRC_COMMON_CLIB_SECRETS_H
#define CLIB_SRC_COMMON_CLIB_SECRETS_H

typedef struct clib_secret_handle* clib_secrets_t;

clib_secrets_t clib_secrets_load_from_file(const char* file);

char* clib_secret_find_for_hostname(clib_secrets_t secrets, const char* hostname);

#endif//CLIB_SRC_COMMON_CLIB_SECRETS_H
