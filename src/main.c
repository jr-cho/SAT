#include <stdio.h>
#include "runner.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    const char *target = argv[1];

    char *cppcheck_args[] = {"cppcheck", "--xml", (char *)target, NULL};
    char *flawfinder_args[] = {"flawfinder", (char *)target, NULL};
    char *gcc_args[] = {"gcc", "-fanalyzer", (char *)target, NULL};
    char *coverity_args[] = {"cov-analyze", (char *)target, NULL}; // example

    run_tool("cppcheck", cppcheck_args, "cppcheck_report.xml");
    run_tool("flawfinder", flawfinder_args, "flawfinder_report.txt");
    run_tool("gcc", gcc_args, "gcc_analyzer_report.txt");
    run_tool("cov-analyze", coverity_args, "coverity_report.txt");

    printf("Static analysis completed.\n");

    return 0;
}
