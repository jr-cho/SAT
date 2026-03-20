#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void vulnerable_function(char *input) {
    char buffer[10];

    // Buffer overflow
    strcpy(buffer, input);

    // Memory leak
    char *ptr = malloc(100);

    // Use of uninitialized variable
    int x;
    if (x > 0) {
        printf("x is positive\n");
    }

    // Null pointer dereference
    int *p = NULL;
    *p = 5;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        vulnerable_function(argv[1]);
    }
    return 0;
}
