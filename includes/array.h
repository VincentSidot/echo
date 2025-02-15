#include <assert.h>

// Reallocate memory for the dynamic array (must have the same signature as
// realloc)
#ifndef _DA_REALLOC
#include <stdlib.h>
#define _DA_REALLOC realloc
#endif

// Allocate memory for the dynamic array (must have the same signature as
// malloc)
#ifndef _DA_MALLOC
#include <stdlib.h>
#define _DA_MALLOC malloc
#endif

// Copy memory from one location to another (must have the same signature as
// memcpy)
#ifndef _DA_MEMCPY
#include <string.h>
#define _DA_MEMCPY memcpy
#endif

// Initial capacity of the dynamic array
#ifndef _DA_INIT_CAPACITY
#define _DA_INIT_CAPACITY 1
#endif

// Resize the dynamic array to the new capacity
#define _da_realloc(da)                                                        \
  do {                                                                         \
    if ((da)->items == NULL) {                                                 \
      (da)->items = _DA_MALLOC((da)->capacity * sizeof(*(da)->items));         \
    } else {                                                                   \
      (da)->items =                                                            \
          _DA_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));     \
    }                                                                          \
    assert(((da)->items != NULL) && "Maybe you should buy more RAM");          \
  } while (0)

// Increase the capacity of the dynamic array by doubling it
#define _da_increase(da)                                                       \
  do {                                                                         \
    if ((da)->capacity == 0) {                                                 \
      (da)->capacity = _DA_INIT_CAPACITY;                                      \
    } else {                                                                   \
      (da)->capacity <<= 1;                                                    \
    }                                                                          \
  } while (0)

// Append an item to the dynamic array
#define da_append(da, item)                                                    \
  do {                                                                         \
    if ((da)->count >= (da)->capacity) {                                       \
      _da_increase(da);                                                        \
      _da_realloc(da);                                                         \
    }                                                                          \
    (da)->items[(da)->count++] = item;                                         \
  } while (0)

// Resize the dynamic array to the new count
#define da_resize(da, count)                                                   \
  do {                                                                         \
    if ((da)->capacity < (count)) {                                            \
      (da)->capacity = (count);                                                \
      _da_realloc(da);                                                         \
    }                                                                          \
  } while (0)

// Append multiple items to the dynamic array
#define da_append_many(da, _items, _count)                                     \
  do {                                                                         \
    if ((da)->count + (_count) > (da)->capacity) {                             \
      while ((da)->count + (_count) > (da)->capacity) {                        \
        _da_increase(da);                                                      \
      }                                                                        \
      _da_realloc(da);                                                         \
    }                                                                          \
    _DA_MEMCPY((da)->items + (da)->count, (_items),                            \
               (_count) * sizeof(*(da)->items));                               \
    (da)->count += (_count);                                                   \
  } while (0)

// Free the dynamic array
#define da_free(da)                                                            \
  do {                                                                         \
    if ((da)->items != NULL)                                                   \
      free((da)->items);                                                       \
    (da)->items = NULL;                                                        \
    (da)->count = 0;                                                           \
    (da)->capacity = 0;                                                        \
  } while (0)

// Clear the dynamic array
#define da_clear(da)                                                           \
  do {                                                                         \
    (da)->count = 0;                                                           \
  } while (0)

// Remove an item from the dynamic array (**does not preserve order**)
#define da_fast_remove(da, index)                                              \
  do {                                                                         \
    (da)->items[(index)] = (da)->items[((da)->count--) - 1];                   \
  } while (0)

// Remove an item from the dynamic array
#define da_remove(da, index)                                                   \
  do {                                                                         \
    _DA_MEMCPY((da)->items + index, (da)->items + index + 1,                   \
               (--(da)->count) - index);                                       \
  } while (0)

// Shrink the dynamic array to the count item
#define da_shrink(da)                                                          \
  do {                                                                         \
    (da)->capacity = (da)->count;                                              \
    _da_realloc(da);                                                           \
  } while (0)

// Define the dynamic array structure elements for a given type
#define da_struct(type)                                                        \
  size_t count;                                                                \
  size_t capacity;                                                             \
  type *items;
