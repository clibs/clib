
//
// iterator.c
//
// Copyright (c) 2010 TJ Holowaychuk <tj@vision-media.ca>
//

#include "list.h"

/*
 * Allocate a new list_iterator_t. NULL on failure.
 * Accepts a direction, which may be LIST_HEAD or LIST_TAIL.
 */

list_iterator_t *
list_iterator_new(list_t *list, list_direction_t direction) {
  list_node_t *node = direction == LIST_HEAD
    ? list->head
    : list->tail;
  return list_iterator_new_from_node(node, direction);
}

/*
 * Allocate a new list_iterator_t with the given start
 * node. NULL on failure.
 */

list_iterator_t *
list_iterator_new_from_node(list_node_t *node, list_direction_t direction) {
  list_iterator_t *self;
  if (!(self = LIST_MALLOC(sizeof(list_iterator_t))))
    return NULL;
  self->next = node;
  self->direction = direction;
  return self;
}

/*
 * Return the next list_node_t or NULL when no more
 * nodes remain in the list.
 */

list_node_t *
list_iterator_next(list_iterator_t *self) {
  list_node_t *curr = self->next;
  if (curr) {
    self->next = self->direction == LIST_HEAD
      ? curr->next
      : curr->prev;
  }
  return curr;
}

/*
 * Free the list iterator.
 */

void
list_iterator_destroy(list_iterator_t *self) {
  LIST_FREE(self);
  self = NULL;
}
