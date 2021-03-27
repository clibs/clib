//
// github-registry.c
//
// Copyright (c) 2021 Elbert van de Put
// Based on work by Stephen Mathieson
// MIT licensed
//
#include "github-registry.h"
#include "case/case.h"
#include "gumbo-get-element-by-id/get-element-by-id.h"
#include "gumbo-get-elements-by-tag-name/get-elements-by-tag-name.h"
#include "gumbo-text-content/gumbo-text-content.h"
#include "http-get/http-get.h"
#include "registry-internal.h"
#include "strdup/strdup.h"
#include "substr/substr.h"
#include "trim/trim.h"
#include <curl/curl.h>
#include <string.h>

#define GITHUB_BASE_URL "https://github.com/"

/**
 * Add `href` to the given `package`.
 */
static void add_package_href(registry_package_ptr_t self) {
    size_t len = strlen(self->id) + 20; // https://github.com/ \0
    self->href = malloc(len);
    if (self->href)
        sprintf(self->href, GITHUB_BASE_URL "%s", self->id);
}

/**
 * Parse the given wiki `li` into a package.
 */
static registry_package_ptr_t parse_li(GumboNode *li) {
  registry_package_ptr_t self = registry_package_new();
    char *text = NULL;

    if (!self) goto cleanup;

    text = gumbo_text_content(li);
    if (!text) goto cleanup;

    // TODO support unicode dashes
    char *tok = strstr(text, " - ");
    if (!tok) goto cleanup;

    int pos = tok - text;
    self->id = substr(text, 0, pos);
    self->description = substr(text, pos + 3, -1);
    if (!self->id || !self->description) goto cleanup;
    trim(self->description);
    trim(self->id);

    add_package_href(self);

    cleanup:
    free(text);
    return self;
}

/**
 * Parse a list of packages from the given `html`
 */
list_t *wiki_registry_parse(const char *html) {
    GumboOutput *output = gumbo_parse(html);
    list_t *pkgs = list_new();

    GumboNode *body = gumbo_get_element_by_id("wiki-body", output->root);
    if (body) {
        // grab all category `<h2 />`s
        list_t *h2s = gumbo_get_elements_by_tag_name("h2", body);
        list_node_t *heading_node;
        list_iterator_t *heading_iterator = list_iterator_new(h2s, LIST_HEAD);
        while ((heading_node = list_iterator_next(heading_iterator))) {
            GumboNode *heading = (GumboNode *) heading_node->val;
            char *category = gumbo_text_content(heading);
            // die if we failed to parse a category, as it's
            // almost certinaly a malloc error
            if (!category) break;
            trim(case_lower(category));
            GumboVector *siblings = &heading->parent->v.element.children;
            size_t pos = heading->index_within_parent;

            // skip elements until the UL
            // TODO: don't hardcode position here
            // 2:
            //   1 - whitespace
            //   2 - actual node
            GumboNode *ul = siblings->data[pos + 2];
            if (GUMBO_TAG_UL != ul->v.element.tag) {
                free(category);
                continue;
            }

            list_t *lis = gumbo_get_elements_by_tag_name("li", ul);
            list_iterator_t *li_iterator = list_iterator_new(lis, LIST_HEAD);
            list_node_t *li_node;
            while ((li_node = list_iterator_next(li_iterator))) {
              registry_package_ptr_t package = parse_li(li_node->val);
                if (package && package->description) {
                    package->category = strdup(category);
                    list_rpush(pkgs, list_node_new(package));
                } else {
                    // failed to parse package
                    if (package)
                      registry_package_free(package);
                }
            }
            list_iterator_destroy(li_iterator);
            list_destroy(lis);
            free(category);
        }
        list_iterator_destroy(heading_iterator);
        list_destroy(h2s);
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return pkgs;
}

/**
 * Get a list of packages from the given GitHub wiki `url`.
 */
list_t *github_registry_fetch(const char *url) {
    http_get_response_t *res = http_get(url, NULL, 0);
    if (!res->ok) return NULL;

    list_t *list = wiki_registry_parse(res->data);
    http_get_free(res);
    return list;
}
