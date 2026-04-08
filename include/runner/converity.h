#ifndef RUNNER_COVERITY_H
#define RUNNER_COVERITY_H

/*
 * Run Coverity (cov-analyze) on the given source file.
 * Output is written to output_file.
 * Returns the tool's exit code, or -1 on failure.
 */
int run_coverity(const char *source_file, const char *output_file);

#endif
