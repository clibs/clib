
//
// wiki-registry.c
//
// Copyright (c) 2014 Stephen Mathieson
// MIT licensed
//

#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include "gumbo-parser/gumbo.h"
#include "gumbo-text-content/gumbo-text-content.h"
#include "gumbo-get-element-by-id/get-element-by-id.h"
#include "gumbo-get-elements-by-tag-name/get-elements-by-tag-name.h"
#include "http-get/http-get.h"
#include "list/list.h"
#include "substr/substr.h"
#include "strdup/strdup.h"
#include "case/case.h"
#include "trim/trim.h"
#include "wiki-registry.h"

//
// TODO find dox on gumbo so the node iteration isn't so ugly
//

/**
 * Create a new wiki package.
 */

static wiki_package_t *
wiki_package_new() {
  wiki_package_t *pkg = malloc(sizeof(wiki_package_t));
  if (pkg) {
    pkg->repo = NULL;
    pkg->href = NULL;
    pkg->description = NULL;
    pkg->category = NULL;
  }
  return pkg;
}

/**
 * Add `href` to the given `package`.
 */

static void
add_package_href(wiki_package_t *self) {
  size_t len = strlen(self->repo) + 20; // https://github.com/ \0
  self->href = malloc(len);
  if (self->href)
    sprintf(self->href, "https://github.com/%s", self->repo);
}

/**
 * Parse the given wiki `li` into a package.
 */

static wiki_package_t *
parse_li(GumboNode *li) {
  wiki_package_t *self = wiki_package_new();
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

  add_package_href(self);

cleanup:
  free(text);
  return self;
}

/**
 * Parse a list of packages from the given `html`
 */

list_t *
wiki_registry_parse(const char *html) {
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
        wiki_package_t *package = parse_li(li_node->val);
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
 * Get a list of packages from the given GitHub wiki `url`.
 */

list_t *
wiki_registry(const char *url) {
  http_get_response_t *res = http_get(url);
  if (!res->ok) return NULL;

  list_t *list = wiki_registry_parse(res->data);
  http_get_free(res);
  return list;
}

/**
 * Free a wiki_package_t.
 */

void
wiki_package_free(wiki_package_t *pkg) {
  free(pkg->repo);
  free(pkg->href);
  free(pkg->description);
  free(pkg->category);
  free(pkg);
}
