#ifndef __REGEX_WRAPPER_H__
#define  __REGEX_WRAPPER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

/**
 * Compile the regular expression described by "regex_text" into "r".
 */

int regex_compile (regex_t * r, const char * regex_text);

/**
 * Match the string in "to_match" against the compiled regular expression in "r".
 */

int regex_parse (regex_t * r, const char * to_match);

#endif
