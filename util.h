#ifndef LOKICAT_UTIL_H
#define LOKICAT_UTIL_H

#include "lokicat.h"
#include "logproto/logproto.pb-c.h"

// returns char* start address and length pointer
char *trim_whitespace(char *str, size_t *length);

void logproto__entry__free(Logproto__Entry *entry);


#endif  /* LOKICAT_UTIL_H */
