#ifndef GROWING_ARRAY_H
#define GROWING_ARRAY_H

#include <sys/types.h>

struct growing_array;

struct growing_array *create_growing_array(size_t initial_size);
void add_to_growing_array(struct growing_array *ga, void *data);
size_t size_of_growing_array(struct growing_array *ga);
void *get_element_in_growing_array(struct growing_array *ga, size_t index);
void destroy_growing_array(struct growing_array *ga);

#endif