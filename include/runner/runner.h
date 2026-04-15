#ifndef RUNNER_RUNNER_H
#define RUNNER_RUNNER_H

/*
 * Generic tool runner: forks a child process, executes the tool,
 * and redirects stdout+stderr to output_file.
 * Returns the tool's exit code, or -1 on fork/exec failure.
 */
int run_tool(const char *tool, char *const args[], const char *output_file);

#endif
