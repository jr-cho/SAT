#include "../../include/runner/flawfinder.h"
#include "../../include/runner/runner.h"
#include <stddef.h>

int run_flawfinder(const char *source_file, const char *output_file)
{
    char *args[] = {
        "flawfinder",
        "--dataonly",
        "--csv",
        (char *)source_file,
        NULL
    };

    return run_tool("flawfinder", args, output_file);
}
