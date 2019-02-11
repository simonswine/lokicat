#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "util.h"


// returns char* start address and length pointer
char *trim_whitespace(char *str, size_t *length)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) {
        str++;
    }

    *length = strlen(str);

    if (*str == 0) { // All spaces?
        return str;
    }


    // Trim trailing space
    end = str + *length - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *length = *length - 1;
        end = str + *length - 1;
    }

    return str;
}


void logproto__entry__free(Logproto__Entry *entry)
{
    if (entry->timestamp != NULL) {
        free(entry->timestamp);
    }
    if (entry->line != NULL) {
        free(entry->line);
    }
}
