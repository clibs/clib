
//
// wiki-registry.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#include <string.h>
#include <stdlib.h>
#include "strdup/strdup.h"
#include "gumbo-parser/gumbo.h"
#include "list/list.h"
#include "wiki-registry-internal.h"
#include "github-registry.h"
#include "gitlab-registry.h"
#include "url/url.h"

enum wiki_registry_type_t {
    REGISTRY_TYPE_GITHUB,
    REGISTRY_TYPE_GITLAB,
};

struct wiki_registry_t {
    enum wiki_registry_type_t type;
    char* url;
    char* hostname;
    list_t *packages;
};

/**
 * Create a new wiki package.
 */
wiki_package_ptr_t wiki_package_new() {
    wiki_package_ptr_t pkg = malloc(sizeof(struct wiki_package_t));
    if (pkg) {
        pkg->repo = NULL;
        pkg->href = NULL;
        pkg->description = NULL;
        pkg->category = NULL;
    }
    return pkg;
}

/**
 * Free a wiki_package_t.
 */
void wiki_package_free(wiki_package_ptr_t pkg) {
    free(pkg->repo);
    free(pkg->href);
    free(pkg->description);
    free(pkg->category);
    free(pkg);
}

wiki_registry_ptr_t wiki_registry_create(const char *url) {
    wiki_registry_ptr_t registry = malloc(sizeof(struct wiki_registry_t));
    registry->url = strdup(url);

    if (strstr(url, "github.com") != NULL) {
        registry->type = REGISTRY_TYPE_GITHUB;
    } else if (strstr(url, "gitlab") != NULL) {
        registry->type = REGISTRY_TYPE_GITLAB;
    } else {
        return NULL;
    }

    url_data_t *parsed = url_parse(url);
    registry->hostname = strdup(parsed->hostname);
    url_free(parsed);

    return registry;
}

void wiki_registry_free(wiki_registry_ptr_t registry) {
    free(registry->url);
    free(registry->hostname);
    if (registry->packages != NULL) {
        list_iterator_t* it = list_iterator_new(registry->packages, LIST_HEAD);
        list_node_t* node;
        while ((node = list_iterator_next(it))) {
            wiki_package_free(node->val);
        }
        list_iterator_destroy(it);
        list_destroy(registry->packages);
    }
    free(registry);
}

// TODO, return false if the request fails.
bool wiki_registry_fetch(wiki_registry_ptr_t registry) {
    switch (registry->type) {
        case REGISTRY_TYPE_GITLAB:
            registry->packages = gitlab_registry_fetch(registry->url, registry->hostname);
            break;
        case REGISTRY_TYPE_GITHUB:
            registry->packages = github_registry_fetch(registry->url);
            break;
        default:
            return false;
    }

    return false;
}


wiki_registry_iterator_t wiki_registry_iterator_new(wiki_registry_ptr_t registry) {
    return list_iterator_new(registry->packages, LIST_HEAD);
}

wiki_package_ptr_t wiki_registry_iterator_next(wiki_registry_iterator_t iterator) {
    list_node_t *node = list_iterator_next(iterator);
    if (node == NULL) {
        return NULL;
    }

    return (wiki_package_ptr_t )node->val;
}

void wiki_registry_iterator_destroy(wiki_registry_iterator_t iterator) {
   list_iterator_destroy(iterator);
}

char* wiki_package_get_repo(wiki_package_ptr_t package) {
    return package->repo;
}

char* wiki_package_get_href(wiki_package_ptr_t package) {
    return package->href;
}

char* wiki_package_get_description(wiki_package_ptr_t package) {
    return package->description;
}

char* wiki_package_get_category(wiki_package_ptr_t package) {
    return package->category;
}