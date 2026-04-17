#include "tool_finder.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Search PATH for an executable, like which(1). Returns 1 if found. */
static int in_path(const char *name)
{
    const char *path_env = getenv("PATH");
    if (!path_env) return 0;

    char path[4096];
    strncpy(path, path_env, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    char probe[512];
    char *dir = strtok(path, ":");
    while (dir) {
        snprintf(probe, sizeof(probe), "%s/%s", dir, name);
        if (access(probe, X_OK) == 0) return 1;
        dir = strtok(NULL, ":");
    }
    return 0;
}

const char *find_gcc_analyzer(void)
{
    const char *env = getenv("GCC_ANALYZER");
    if (env && access(env, X_OK) == 0)
        return env;

    /* Absolute paths are checked directly; bare names are searched via PATH. */
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

    for (int i = 0; candidates[i]; i++) {
        if (candidates[i][0] == '/') {
            if (access(candidates[i], X_OK) == 0)
                return candidates[i];
        } else {
            if (in_path(candidates[i]))
                return candidates[i];
        }
    }

    return NULL;
}
