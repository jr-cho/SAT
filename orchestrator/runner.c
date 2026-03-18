#include "runner.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int run_cppcheck(const char *target_path, const char *output_file) {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open failed");
            exit(1);
        }

        dup2(fd, STDERR_FILENO); // Cppcheck writes XML to stderr
        close(fd);

        char *args[] = {"cppcheck", "--xml", target_path, NULL};
        execvp("cppcheck", args);

        perror("exec failed");
        exit(1);
    }
    else if (pid > 0) {
        // Parent waits
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
    else {
        perror("fork failed");
        return -1;
    }
}
