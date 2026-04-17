#pragma once

#include "../core/database.h"
#include <stdio.h>

void report_print(const Database *db, const char *target, FILE *out);
int  report_write(const Database *db, const char *target, const char *out_path);
