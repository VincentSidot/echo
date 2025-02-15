#include <stdio.h>

#include "../includes/array.h"

#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define test_assert(cond, fmt)                                                 \
  if (!(cond)) {                                                               \
    eprintf("[%s:%d] %s: " COLOR_YELLOW fmt COLOR_RESET "\n", __FILE__,        \
            __LINE__, __func__);                                               \
    return 1;                                                                  \
  }

typedef struct {
  da_struct(int)
} s_da_int;

int test_append() {
  s_da_int da = {0};

  da_append(&da, 1);
  da_append(&da, 2);
  da_append(&da, 3);

  test_assert(da.count == 3, "Count should be 3");
  test_assert(da.capacity == 4, "Capacity should be 4");

  for (size_t i = 0; i < da.count; i++) {
    test_assert(da.items[i] == i + 1, "Item should be equal to index + 1");
  }

  da_free(&da);

  test_assert(da.count == 0, "Count should be 0");
  test_assert(da.capacity == 0, "Capacity should be 0");
  test_assert(da.items == NULL, "Items should be NULL");
  return 0;
}

int test_fast_remove() {
  s_da_int da = {0};

  da_append(&da, 1);
  da_append(&da, 2);
  da_append(&da, 3);

  test_assert(da.count == 3, "Count should be 3");
  test_assert(da.capacity == 4, "Capacity should be 4");

  da_fast_remove(&da, 1);

  test_assert(da.count == 2, "Count should be 2");
  test_assert(da.capacity == 4, "Capacity should be 4");
  test_assert(da.items[1] == 3, "Item should be 3");

  da_free(&da);
  return 0;
}

int test_remove() {
  s_da_int da = {0};

  int items[] = {1, 2, 3, 4, 5};

  da_append_many(&da, items, 5);

  test_assert(da.count == 5, "Count should be 5");
  test_assert(da.capacity == 8, "Capacity should be 8");

  da_remove(&da, 2);

  test_assert(da.count == 4, "Count should be 4");
  test_assert(da.capacity == 8, "Capacity should be 8");

  da_shrink(&da);

  test_assert(da.count == 4, "Count should be 4");
  test_assert(da.capacity == 4, "Capacity should be 4");

  for (size_t i = 0; i < da.count; i++) {
    if (i == 2) {
      continue;
    }
    test_assert(da.items[i] == i + 1, "Item should be equal to index + 1");
  }

  da_free(&da);
  return 0;
}

int main() {

  int failed = 0;

  failed += test_append();
  failed += test_fast_remove();
  failed += test_remove();

  if (failed) {
    eprintf(COLOR_RED "Failed %d tests" COLOR_RESET "\n", failed);
    return 1;
  } else {
    eprintf(COLOR_GREEN "All tests passed" COLOR_RESET "\n");
    return 0;
  }
}