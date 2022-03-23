#ifndef CLIB_GITLAB_REGISTRY_H
#define CLIB_GITLAB_REGISTRY_H

#include "list/list.h"

list_t* gitlab_registry_fetch(const char* url, const char* hostname, const char* secret);

#endif //CLIB_GITLAB_REGISTRY_H
