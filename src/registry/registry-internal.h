//
// registry-internal.h
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
// DO NOT INCLUDE. THIS HEADER IS INTERNAL ONLY
#ifndef REGISTRY_INTERNAL_H
#define REGISTRY_INTERNAL_H
#include "registry.h"

struct registry_package_t {
    char *id;
    char *href;
    char *description;
    char *category;
};

registry_package_ptr_t registry_package_new();

void registry_package_free(registry_package_ptr_t pkg);

#endif
