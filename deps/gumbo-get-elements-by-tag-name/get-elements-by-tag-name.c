
#include <stdlib.h>
#include <string.h>
#include "gumbo-parser/gumbo.h"
#include "list/list.h"
#include "trim/trim.h"
#include "case/case.h"

#define NODE_TYPE_CHECK(node)           \
  if (GUMBO_NODE_DOCUMENT != node->type \
   && GUMBO_NODE_ELEMENT != node->type) \
    return

/**
 * Crawl all children of the given `node`,
 * adding `tag` elements to the given `list_t`.
 */

static void
crawl(GumboTag tag, list_t *elements, GumboNode *node) {
  NODE_TYPE_CHECK(node);

  if (tag == node->v.element.tag)
    list_rpush(elements, list_node_new(node));

  GumboVector *children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; i++) {
    crawl(tag, elements, children->data[i]);
  }
}

/**
 * Get all elements of `tag_name` contained
 * with the given `root` node.
 */

list_t *
gumbo_get_elements_by_tag_name(const char *tag_name, GumboNode *root) {
  GumboTag tag;
  list_t *elements = NULL;

  if (!tag_name || GUMBO_TAG_UNKNOWN == (tag = gumbo_tag_enum(tag_name)))
    return NULL;

  if (!(elements = list_new()))
    return NULL;

  crawl(tag, elements, root);

  return elements;
}
