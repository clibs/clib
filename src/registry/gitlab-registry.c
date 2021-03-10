//
// gitlab-registry.c
//
// Copyright (c) 2020 Elbert van de Put
// MIT licensed
//
#include "gitlab-registry.h"
#include <string.h>
#include "gumbo-text-content/gumbo-text-content.h"
#include "gumbo-get-element-by-id/get-element-by-id.h"
#include "gumbo-get-elements-by-tag-name/get-elements-by-tag-name.h"
#include "http-get/http-get.h"
#include <curl/curl.h>
#include "substr/substr.h"
#include "strdup/strdup.h"
#include "case/case.h"
#include "trim/trim.h"
#include "wiki-registry-internal.h"

/**
 * Add `href` to the given `package`.
 * We assume that all packages listed by a registry live on the same platform as the registry.
 */
static void add_package_href(wiki_package_ptr_t self, const char* hostname) {
    size_t len = strlen(self->repo) + strlen(hostname);
    self->href = malloc(len);
    if (self->href)
        sprintf(self->href, "https://%s/%s", hostname, self->repo);
}

/**
 * Parse the given wiki `li` into a package.
 */
static wiki_package_ptr_t parse_li(GumboNode *li, const char* hostname) {
    wiki_package_ptr_t self = wiki_package_new();
    char *text = NULL;

    if (!self) goto cleanup;

    text = gumbo_text_content(li);
    if (!text) goto cleanup;

    // TODO support unicode dashes
    char *tok = strstr(text, " - ");
    if (!tok) goto cleanup;

    int pos = tok - text;
    self->repo = substr(text, 0, pos);
    self->description = substr(text, pos + 3, -1);
    if (!self->repo || !self->description) goto cleanup;
    trim(self->description);
    trim(self->repo);

    add_package_href(self, hostname);

    cleanup:
    free(text);
    return self;
}

/**
 * Parse a list of packages from the given `html`
 */
static list_t *wiki_registry_parse(const char* hostname, const char *html) {
    list_t *pkgs = list_new();

    // Try to parse the markdown file.
    char* input = strdup(html);
    char* line;
    char* category = NULL;
    while ((line = strsep(&input, "\n"))) {
        char* dash_position = strstr(line, "-");
        // The line starts with a dash, so we expect a package.
        if (dash_position != NULL && dash_position - line < 4) {

            char* link_name_start = strstr(line, "[")+1;
            char* link_name_end = strstr(link_name_start, "]");
            char* link_value_start = strstr(link_name_end, "(")+1;
            char* link_value_end = strstr(link_value_start, ")");
            char* description_position = strstr(link_value_end, "-")+1;

            wiki_package_ptr_t package = wiki_package_new();
            package->href = strndup(link_value_start, link_value_end-link_value_start);
            package->repo = strndup(link_name_start, link_name_end-link_name_start);
            package->description = strdup(description_position);
            package->category = strdup(category != NULL ? category: "unknown");
            list_rpush(pkgs, list_node_new(package));
        }

        char* header_position = strstr(line, "##");
        // The category starts with a ##.
        if (header_position != NULL && header_position - line < 4) {
            category = header_position+2;
        }
    }

    free(input);

    return pkgs;

    GumboOutput *output = gumbo_parse(html);
    GumboNode *body = gumbo_get_element_by_id("content-body", output->root);
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
                wiki_package_ptr_t package = parse_li(li_node->val, hostname);
                if (package && package->description) {
                    package->category = strdup(category);
                    list_rpush(pkgs, list_node_new(package));
                } else {
                    // failed to parse package
                    if (package) wiki_package_free(package);
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
 * Get a list of packages from the given gitlab wiki `url`.
 * TODO, get the secret from a secrets file.
 */
list_t *gitlab_registry_fetch(const char *url, const char* hostname) {
    char* headers[1] = {"PRIVATE-TOKEN: SECRET"};
    http_get_response_t *res = http_get(url, headers, 1);
    if (!res->ok) return NULL;

    list_t *list = wiki_registry_parse(hostname, res->data);
    http_get_free(res);
    return list;
}

