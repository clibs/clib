#ifndef WIKI_REGISTRY_HELPER_H
#define WIKI_REGISTRY_HELPER_H
// DO NOT INCLUDE. THIS HEADER IS INTERNAL ONLY
#include "wiki-registry.h"

struct wiki_package_t {
    char *repo;
    char *href;
    char *description;
    char *category;
};

wiki_package_ptr_t wiki_package_new();

void wiki_package_free(wiki_package_ptr_t pkg);

#endif //WIKI_REGISTRY_HELPER_H
