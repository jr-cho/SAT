#include "../../include/runner/runner.h"
#include "../../include/common/logging.h"
#include <stdlib.h>
#include <string.h>

/* ── Windows ──────────────────────────────────────────────────────────────── */
#ifdef _WIN32
#include <windows.h>

/*
 * Build a properly-quoted CreateProcess command-line string from an argv array.
 * Each token is wrapped in double-quotes with internal quotes and backslashes
 * that precede a quote character escaped.
 */
static int build_cmdline(char *buf, size_t size, char *const args[])
{
    size_t pos = 0;
    for (int i = 0; args[i]; i++) {
        const char *a = args[i];
        if (i > 0) {
            if (pos + 1 >= size) return -1;
            buf[pos++] = ' ';
        }
        if (pos + 1 >= size) return -1;
        buf[pos++] = '"';
        for (; *a; a++) {
            if (*a == '\\' && *(a + 1) == '"') {
                /* Backslash before an embedded quote must itself be escaped. */
                if (pos + 2 >= size) return -1;
                buf[pos++] = '\\';
            }
            if (*a == '"') {
                if (pos + 2 >= size) return -1;
                buf[pos++] = '\\';
            }
            if (pos + 1 >= size) return -1;
            buf[pos++] = *a;
        }
        if (pos + 1 >= size) return -1;
        buf[pos++] = '"';
    }
    if (pos >= size) return -1;
    buf[pos] = '\0';
    return 0;
}

int run_tool(const char *tool, char *const args[], const char *output_file)
{
    char cmdline[32768];
    if (build_cmdline(cmdline, sizeof(cmdline), args) < 0) {
        LOG_ERROR("Command line too long for tool '%s'", tool);
        return -1;
    }

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength        = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hFile = CreateFileA(output_file, GENERIC_WRITE, FILE_SHARE_READ,
                               &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOG_ERROR("Cannot create output file: %s", output_file);
        return -1;
    }

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESTDHANDLES;
    si.hStdOutput = hFile;
    si.hStdError  = hFile;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL,
                             TRUE, 0, NULL, NULL, &si, &pi);
    CloseHandle(hFile);

    if (!ok) {
        LOG_ERROR("Failed to launch '%s' (GetLastError=%lu)",
                  tool, (unsigned long)GetLastError());
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    LOG_INFO("Tool '%s' exited with code %lu, output -> %s",
             tool, (unsigned long)code, output_file);
    return (int)code;
}

/* ── POSIX ────────────────────────────────────────────────────────────────── */
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

int run_tool(const char *tool, char *const args[], const char *output_file)
{
    pid_t pid = fork();

    if (pid == 0) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        execvp(tool, args);
        perror("exec failed");
        exit(1);
    }
    else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
        LOG_INFO("Tool '%s' exited with code %d, output -> %s", tool, code, output_file);
        return code;
    }
    else {
        perror("fork failed");
        return -1;
    }
}

#endif
