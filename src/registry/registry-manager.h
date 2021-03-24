//
// registry-manager.h
//
// Copyright (c) 2021 Elbert van de Put
// MIT licensed
//
#ifndef CLIB_SRC_REGISTRY_REGISTRY_MANAGER_H
#define CLIB_SRC_REGISTRY_REGISTRY_MANAGER_H

#include "clib-secrets.h"
#include "list/list.h"
#include "registry.h"

// Contains an abstraction for handling multiple registries

typedef list_t* registries_t;
typedef list_iterator_t* registry_iterator_t;

/**
 * Initializes all registies specified by the urls
 * @param registry_urls
 * @param secrets
 * @return
 */
registries_t registry_manager_init_registries(list_t* registry_urls, clib_secrets_t secrets);

void registry_manager_fetch_registries(registries_t registries);

/**
 * An iterator through the registries.
 */
registry_iterator_t registry_iterator_new(registries_t registry);
registry_ptr_t registry_iterator_next(registry_iterator_t iterator);
void registry_iterator_destroy(registry_iterator_t iterator);

/**
 * Search the registry for a package
 * @param registry a registry handle
 * @param package_id the identifier of the package "<namespace>/<package_name>"
 * @return a pointer to the package if it could be found or NULL
 */
registry_package_ptr_t registry_manger_find_package(registries_t registries, const char* package_id);

#endif//CLIB_SRC_REGISTRY_REGISTRY_MANAGER_H
