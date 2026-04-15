#ifndef FLAWFINDER_PARSER_H
#define FLAWFINDER_PARSER_H

#include "parser.h"

/*
 * Parse a flawfinder --dataonly --csv output file.
 * Returns a FindingList, or NULL on error.
 */
FindingList *parse_flawfinder(const char *output_file);

#endif
