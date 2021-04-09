//
// registry.h
//
// Copyright (c) 2020 clib authors
// MIT licensed
//

#ifndef REGISTRY_H
#define REGISTRY_H 1

#include <stdbool.h>

typedef struct registry_package_t * registry_package_ptr_t;
typedef struct registry_t * registry_ptr_t;
typedef list_iterator_t* registry_package_iterator_t;

/**
 * Create a new registry for the given url.
 * @param url the url of the registry.
 * @param secret the secret to authenticate with this registry of NULL if no secret is required.
 * @return a handle to the registry
 */
registry_ptr_t registry_create(const char* url, const char* secret);

/**
 * Free the memory held by the registry.
 * @param registry
 */
void registry_free(registry_ptr_t registry);

/**
 * Fetch the list of packages from the registry.
 * @param registry
 */
bool registry_fetch(registry_ptr_t registry);

/**
 * Get the url for the registry
 * @param registry
 * @return the url
 */
const char* registry_get_url(registry_ptr_t registry);

/**
 * Get the secret for this registry
 * @param registry
 * @return the secret or NULL if there is no secret.
 */
const char* registry_get_secret(registry_ptr_t registry);

/**
 * An iterator through the packages in the registry.
 */
registry_package_iterator_t registry_package_iterator_new(registry_ptr_t registry);
registry_package_ptr_t registry_package_iterator_next(registry_package_iterator_t iterator);
void registry_package_iterator_destroy(registry_package_iterator_t iterator);

/**
 * Search the registry for a package
 * @param registry a registry handle
 * @param package_id the identifier of the package "<namespace>/<package_name>"
 * @return a pointer to the package if it could be found or NULL
 */
registry_package_ptr_t registry_find_package(registry_ptr_t registry, const char* package_id);

char* registry_package_get_id(registry_package_ptr_t package);
char* registry_package_get_href(registry_package_ptr_t package);
char* registry_package_get_description(registry_package_ptr_t package);
char* registry_package_get_category(registry_package_ptr_t package);

#endif