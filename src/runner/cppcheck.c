#include "../../include/runner/cppcheck.h"
#include "../../include/runner/runner.h"
#include <stddef.h>

int run_cppcheck(const char *source_file, const char *output_file)
{
    char *args[] = {
        "cppcheck",
        "--xml",
        "--xml-version=2",
        "--enable=all",
        "--suppress=missingIncludeSystem",
        (char *)source_file,
        NULL
    };

    return run_tool("cppcheck", args, output_file);
}
