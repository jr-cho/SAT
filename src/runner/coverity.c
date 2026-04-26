#include "../../include/runner/converity.h"
#include "../../include/runner/runner.h"
#include "../../include/common/logging.h"

int run_coverity(const char *source_file, const char *output_file)
{
    /*
     * Coverity requires a build capture step (cov-build) before analysis.
     * This runner calls cov-analyze directly; in a real setup you would
     * first run: cov-build --dir cov-int <build command>
     * then:       cov-analyze --dir cov-int
     */
    char *args[] = {
        "cov-analyze",
        "--dir", "cov-int",
        "--",
        (char *)source_file,
        NULL
    };

    LOG_WARN("Coverity requires a prior cov-build step; results may be incomplete");
    return run_tool("cov-analyze", args, output_file);
}
