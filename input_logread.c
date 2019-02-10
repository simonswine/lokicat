#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lokicat.h"
#include "logproto/logproto.pb-c.h"

lokicat_input_status input_logread(FILE *stream, Logproto__Entry *entry)
{
    char *line = NULL;
    size_t len = 0;
    size_t read;
    struct tm tm = {0};

    while ((read = getline(&line, &len, stream)) != (size_t) -1) {
        char *res = strptime(line, "%a %b %e %H:%M:%S %Y", &tm);
        if (res == NULL) {
            res = line;
        } else {
            // timestamp parsed
            entry->timestamp = malloc(sizeof(Google__Protobuf__Timestamp));
            google__protobuf__timestamp__init(entry->timestamp);
            entry->timestamp->seconds = mktime(&tm);
        }

        // TODO: parse PID, process name, severity

        // ensure string is copied
        entry->line = malloc(strlen(res) + 1);
        strcpy(entry->line, res);
        free(line);
        return lokicat_input_status_success;
    }

    free(line);
    return lokicat_input_status_eof;
}
