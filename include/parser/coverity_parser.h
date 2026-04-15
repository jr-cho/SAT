#ifndef COVERITY_PARSER_H
#define COVERITY_PARSER_H

#include "parser.h"

/*
 * Parse cov-analyze text output.
 * Returns a FindingList, or NULL on error.
 */
FindingList *parse_coverity(const char *output_file);

#endif
