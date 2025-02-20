//usr/bin/gcc "$0" -o "${0%.c}" && ${0%.c} $@ ; rm -f "${0%.c}"; exit
#include <stdio.h>
#include <string.h>

void main() {
    int array[] = {1, 2, 3, 4, 5};

    for (int i = 0; i < 5; i++) {
        printf("array[%d] = %d\n", i, array[i]);
    }
    puts("=========");

    memmove(array + 2, array, 2*sizeof(*array));
    
    for (int i = 0; i < 5; i++) {
        printf("array[%d] = %d\n", i, array[i]);
    }

}