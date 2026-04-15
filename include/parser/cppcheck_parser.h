#ifndef CPPCHECK_PARSER_H
#define CPPCHECK_PARSER_H

#include "parser.h"

/*
 * Parse a cppcheck --xml --xml-version=2 output file.
 * Returns a FindingList, or NULL on error.
 */
FindingList *parse_cppcheck(const char *output_file);

#endif
