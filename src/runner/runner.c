#include "../../include/runner/runner.h"
#include "../../include/common/logging.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>

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
        int code = WEXITSTATUS(status);
        LOG_INFO("Tool '%s' exited with code %d, output -> %s", tool, code, output_file);
        return code;
    }
    else {
        perror("fork failed");
        return -1;
    }
}
