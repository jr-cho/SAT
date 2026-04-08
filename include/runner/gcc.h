#ifndef RUNNER_GCC_H
#define RUNNER_GCC_H

/*
 * Run GCC with -fanalyzer on the given source file.
 * Output is written to output_file.
 * Returns the tool's exit code, or -1 on failure.
 */
int run_gcc_analyzer(const char *source_file, const char *output_file);

#endif
