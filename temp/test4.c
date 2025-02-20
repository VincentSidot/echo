//usr/bin/gcc "$0" -o "${0%.c}" && ${0%.c} $@ ; rm -f "${0%.c}"; exit
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    // Send the escape sequence to get the cursor position
    printf("\033[6n");
    fflush(stdout);

    // Read the response from the terminal
    char response[32];
    fgets(response, sizeof(response), stdin);

    // Parse the response to get the row and column
    int row, col;
    if (sscanf(response, "\033[%d;%dR", &row, &col) == 2) {
        printf("Cursor position: Row = %d, Column = %d\n", row, col);
    } else {
        printf("Failed to get cursor position\n");
    }

    return 0;
}