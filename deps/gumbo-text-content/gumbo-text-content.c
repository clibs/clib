
#include <stdlib.h>
#include <string.h>
#include "gumbo-parser/gumbo.h"
#include "gumbo-text-content.h"

/**
 * Maximum number of text nodes.
 */

#define GUMBO_TEXT_CONTENT_MAX 1024

typedef struct {
  const char *nodes[GUMBO_TEXT_CONTENT_MAX];
  size_t length;
} text_nodes_t;

/**
 * Get a `text_nodes_t` instance of all text nodes
 * contained in given `root` node.
 */

static text_nodes_t *
get_text_nodes(GumboNode *root) {
  text_nodes_t *nodes = malloc(sizeof(text_nodes_t));
  if (!nodes) return NULL;
  nodes->length = 0;

  GumboVector *children = &root->v.element.children;

  for (size_t i = 0; i < children->length; i++) {
    GumboNode *child = children->data[i];
    if (GUMBO_NODE_TEXT == child->type) {
      nodes->nodes[nodes->length++] = child->v.text.text;
    } else if (GUMBO_NODE_ELEMENT == child->type) {
      text_nodes_t *child_nodes = get_text_nodes(child);
      // exit loop on malloc failure
      if (!child_nodes) break;
      // join children with our node
      if (child_nodes->length) {
        for (size_t j = 0; j < child_nodes->length; j++) {
          nodes->nodes[nodes->length++] = child_nodes->nodes[j];
        }
      }
      free(child_nodes);
    }
  }
  return nodes;
}


/**
 * Get all text contained in the given `root` node, much
 * like `Node#textContent`.
 */

char *
gumbo_text_content(GumboNode *node) {
  text_nodes_t *text_nodes = NULL;
  char *text_content = NULL;
  size_t length = 1;
  int pos = 0;

  text_nodes = get_text_nodes(node);
  if (!text_nodes) goto cleanup;

  // calculate total length of all text
  for (size_t i = 0; i < text_nodes->length; i++) {
    length += strlen(text_nodes->nodes[i]);
  }

  text_content = malloc(length);
  if (!text_content) goto cleanup;
  *text_content = '\0';
  // join all text nodes
  for (size_t i = 0; i < text_nodes->length; i++) {
    size_t l = strlen(text_nodes->nodes[i]);
    strncat(&text_content[pos], text_nodes->nodes[i], l);
    pos += l;
    text_content[pos] = '\0';
  }

cleanup:
  free(text_nodes);
  return text_content;
}
