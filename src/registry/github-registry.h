#ifndef CLIB_GITHUB_REGISTRY_H
#define CLIB_GITHUB_REGISTRY_H

#include "list/list.h"

list_t* github_registry_fetch(const char *url);

#endif //CLIB_GITHUB_REGISTRY_H
