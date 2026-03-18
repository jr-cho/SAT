#include "runner.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int run_tool(const char *tool, char *const args[], const char *output_file)
{
    pid_t pid = fork();

    if (pid == 0) {
        // Child process

        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(1);
        }

        // Redirect stderr to file
        dup2(fd, STDERR_FILENO);

        // Optional: redirect stdout as well
        dup2(fd, STDOUT_FILENO);

        close(fd);

        execvp(tool, args);

        perror("exec failed");
        exit(1);
    }
    else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    else {
        perror("fork failed");
        return -1;
    }
}
