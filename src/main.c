#include <stdio.h>
#include "runner.h"
#include "tool_finder.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    const char *target = argv[1];
    const char *gcc_exec = find_gcc_analyzer();

    char *cppcheck_args[] = {"cppcheck", "--xml", (char *)target, NULL};
    char *flawfinder_args[] = {"flawfinder", (char *)target, NULL};
    char *coverity_args[] = {"cov-analyze", (char *)target, NULL};

    run_tool("cppcheck", cppcheck_args, "cppcheck_report.xml");
    run_tool("flawfinder", flawfinder_args, "flawfinder_report.txt");

    if (gcc_exec) {
        char *gcc_args[] = {(char *)gcc_exec, "-fanalyzer", (char *)target, NULL};
        run_tool(gcc_exec, gcc_args, "gcc_analyzer_report.txt");
    } else {
        printf("GNU GCC analyzer not found, skipping GCC analyzer.\n");
    }

    run_tool("cov-analyze", coverity_args, "coverity_report.txt");

    printf("Static analysis completed.\n");
    return 0;
}