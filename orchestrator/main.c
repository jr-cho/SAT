#include <stdio.h>
#include "runner.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    const char *target = argv[1];
    int status = run_cppcheck(target, "cppcheck_report.xml");

    if (status == 0) {
        printf("Cppcheck completed successfully.\n");
    } else {
        printf("Cppcheck finished with errors.\n");
    }

    return 0;
}
