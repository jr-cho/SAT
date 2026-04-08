#include "../../include/runner/gcc.h"
#include "../../include/runner/runner.h"
#include "../../include/common/logging.h"
#include "../../src/tool_finder.h"

int run_gcc_analyzer(const char *source_file, const char *output_file)
{
    const char *gcc = find_gcc_analyzer();
    if (!gcc) {
        LOG_WARN("GCC analyzer not found, skipping");
        return -1;
    }

    char *args[] = {
        (char *)gcc,
        "-fanalyzer",
        "-fanalyzer-output=text",
        "-o", "/dev/null",
        (char *)source_file,
        NULL
    };

    return run_tool(gcc, args, output_file);
}
