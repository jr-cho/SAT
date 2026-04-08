#ifndef RUNNER_FLAWFINDER_H
#define RUNNER_FLAWFINDER_H

/*
 * Run flawfinder on the given source file.
 * Output is written to output_file.
 * Returns the tool's exit code, or -1 on failure.
 */
int run_flawfinder(const char *source_file, const char *output_file);

#endif
