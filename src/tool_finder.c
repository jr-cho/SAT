#include "tool_finder.h"
#include <unistd.h>
#include <stdlib.h>

const char *find_gcc_analyzer(void) {
    const char *env = getenv("GCC_ANALYZER");
    if (env && access(env, X_OK) == 0) {
        return env;
    }

    static const char *candidates[] = {
        "gcc-15",
        "gcc-14",
        "gcc-13",
        "/opt/homebrew/bin/gcc-15",
        "/opt/homebrew/bin/gcc-14",
        "/opt/homebrew/bin/gcc-13",
        "/usr/local/bin/gcc-15",
        "/usr/local/bin/gcc-14",
        "/usr/local/bin/gcc-13",
        "gcc",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            return candidates[i];
        }
    }

    return NULL;
}