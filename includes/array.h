#include <assert.h>

#ifdef _DA_THREAD_SAFE
#include <pthread.h>
#define _DA_MUTEX pthread_mutex_t lock;
#define _da_lock(da) pthread_mutex_lock(&(da)->lock)
#define _da_unlock(da) pthread_mutex_unlock(&(da)->lock)
#define _da_init(da) pthread_mutex_init(&(da)->lock, NULL)
#define _da_destroy(da) pthread_mutex_destroy(&(da)->lock)
#else
#define _DA_MUTEX
#define _da_lock(da)
#define _da_unlock(da)
#define _da_init(da)
#define _da_destroy(da)
#endif

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

// Append an item to the dynamic array without locking
#define da_append_unsafe(da, item)                                             \
  do {                                                                         \
    if ((da)->count >= (da)->capacity) {                                       \
      _da_increase(da);                                                        \
      _da_realloc(da);                                                         \
    }                                                                          \
    (da)->items[(da)->count++] = item;                                         \
  } while (0)

// Append an item to the dynamic array
#define da_append(da, item)                                                    \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_append_unsafe(da, item);                                                \
    _da_unlock(da);                                                            \
  } while (0)

// Resize the dynamic array to the new target without locking
#define da_resize_unsafe(da, _capacity)                                        \
  do {                                                                         \
    if ((da)->capacity < (_capacity)) {                                        \
      (da)->capacity = (_capacity);                                            \
      _da_realloc(da);                                                         \
    }                                                                          \
  } while (0)

// Resize the dynamic array to the new target
#define da_resize(da, _capacity)                                               \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_resize_unsafe(da, _capacity);                                           \
    _da_unlock(da);                                                            \
  } while (0)

// Append multiple items to the dynamic array without locking
#define da_append_many_unsafe(da, _items, _count)                              \
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

// Append multiple items to the dynamic array
#define da_append_many(da, _items, _count)                                     \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_append_many_unsafe(da, _items, _count);                                 \
    _da_unlock(da);                                                            \
  } while (0)

// Free the dynamic array without locking
#define da_free_unsafe(da)                                                     \
  do {                                                                         \
    if ((da)->items != NULL)                                                   \
      free((da)->items);                                                       \
    (da)->items = NULL;                                                        \
    (da)->count = 0;                                                           \
    (da)->capacity = 0;                                                        \
  } while (0)

// Free the dynamic array
#define da_free(da)                                                            \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_free_unsafe(da);                                                        \
    _da_unlock(da);                                                            \
    _da_destroy(da);                                                           \
  } while (0)

// Clear the dynamic array without locking
#define da_clear_unsafe(da)                                                    \
  do {                                                                         \
    (da)->count = 0;                                                           \
  } while (0)

// Clear the dynamic array
#define da_clear(da)                                                           \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_clear_unsafe(da);                                                       \
    _da_unlock(da);                                                            \
  } while (0)

// Remove an item from the dynamic array without locking (**does not preserve
// order**)
#define da_fast_remove_unsafe(da, index)                                       \
  do {                                                                         \
    (da)->items[(index)] = (da)->items[((da)->count--) - 1];                   \
  } while (0)

// Remove an item from the dynamic array (**does not preserve order**)
#define da_fast_remove(da, index)                                              \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_fast_remove_unsafe(da, index);                                          \
    _da_unlock(da);                                                            \
  } while (0)

// Remove an item from the dynamic array without locking
#define da_remove_unsafe(da, index)                                            \
  do {                                                                         \
    _DA_MEMCPY((da)->items + index, (da)->items + index + 1,                   \
               (--(da)->count) - index);                                       \
  } while (0)

// Remove an item from the dynamic array
#define da_remove(da, index)                                                   \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_remove_unsafe(da, index);                                               \
    _da_unlock(da);                                                            \
  } while (0)

// Shrink the dynamic array to the count item without locking
#define da_shrink_unsafe(da)                                                   \
  do {                                                                         \
    (da)->capacity = (da)->count;                                              \
    _da_realloc(da);                                                           \
  } while (0)

// Shrink the dynamic array to the count item
#define da_shrink(da)                                                          \
  do {                                                                         \
    _da_lock(da);                                                              \
    da_shrink_unsafe(da);                                                      \
    _da_unlock(da);                                                            \
  } while (0)

#define da_init(da)                                                            \
  do {                                                                         \
    (da)->count = 0;                                                           \
    (da)->capacity = 0;                                                        \
    (da)->items = NULL;                                                        \
    _da_init(da);                                                              \
  } while (0)

// Define the dynamic array structure elements for a given type
#define da_struct(type)                                                        \
  size_t count;                                                                \
  size_t capacity;                                                             \
  _DA_MUTEX                                                                    \
  type *items;
