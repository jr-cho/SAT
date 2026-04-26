#include "tool_finder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Platform shims ──────────────────────────────────────────────────────── */
#ifdef _WIN32
#  include <io.h>
#  ifdef _MSC_VER
     /* MSVC ships strtok_s with the same signature as POSIX strtok_r. */
#    define strtok_r strtok_s
#  endif
   /* Windows has no execute-permission bit; existence check is sufficient. */
#  define can_exec(p)  (_access((p), 0) == 0)
#  define PATH_SEP     ";"
#else
#  include <unistd.h>
#  define can_exec(p)  (access((p), X_OK) == 0)
#  define PATH_SEP     ":"
#endif

/* Search PATH for an executable, like which(1). Returns 1 if found. */
static int in_path(const char *name)
{
    const char *path_env = getenv("PATH");
    if (!path_env) return 0;

    char path[4096];
    strncpy(path, path_env, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    char  probe[512];
    char *save;
    char *dir = strtok_r(path, PATH_SEP, &save);
    while (dir) {
        snprintf(probe, sizeof(probe), "%s/%s", dir, name);
        if (can_exec(probe)) return 1;
        dir = strtok_r(NULL, PATH_SEP, &save);
    }
    return 0;
}

const char *find_gcc_analyzer(void)
{
    const char *env = getenv("GCC_ANALYZER");
    if (env && can_exec(env))
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
            if (can_exec(candidates[i]))
                return candidates[i];
        } else {
            if (in_path(candidates[i]))
                return candidates[i];
        }
    }

    return NULL;
}
