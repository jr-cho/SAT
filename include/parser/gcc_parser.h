#ifndef GCC_PARSER_H
#define GCC_PARSER_H

#include "parser.h"

/*
 * Parse GCC -fanalyzer stderr output (file:line:col: type: message format).
 * Returns a FindingList, or NULL on error.
 */
FindingList *parse_gcc(const char *output_file);

#endif
