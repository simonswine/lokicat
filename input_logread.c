#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lokicat.h"
#include "util.h"
#include "logproto/logproto.pb-c.h"

// take a logread line and parse it to entry
void input_logread_parse_line(char *line, Logproto__Entry *entry)
{
    struct tm tm = {0};
    char *res;

    if ((res = (char *) strptime(line, "%a %b %e %H:%M:%S %Y", &tm)) != NULL) {
        // openwrt format
        entry->timestamp = malloc(sizeof(Google__Protobuf__Timestamp));
        google__protobuf__timestamp__init(entry->timestamp);
        entry->timestamp->seconds = mktime(&tm);
    } else if ((res = (char *) strptime(line, "%b %e %H:%M:%S", &tm)) != NULL) {
        // get today's time
        time_t today_time;
        struct tm *today;
        time(&today_time);
        today = localtime(&today_time);

        // set current year to tm
        tm.tm_year = today->tm_year;
        if (mktime(&tm) > mktime(today)) {
            tm.tm_year = today->tm_year - 1;
        }

        // default journalctl format
        entry->timestamp = malloc(sizeof(Google__Protobuf__Timestamp));
        google__protobuf__timestamp__init(entry->timestamp);
        entry->timestamp->seconds = mktime(&tm);

    } else {
        // time parsing failed
        // TODO: might need current time
        res = line;
    }

    // trim
    size_t length;
    res = trim_whitespace(res, &length);

    // ensure string is copied
    entry->line = malloc(length + 1);
    strncpy(entry->line, res, length);
    entry->line[length] = '\0';

    // TODO: parse PID, process name, severity
}

lokicat_input_status input_logread(FILE *stream, Logproto__Entry *entry)
{
    char *line = NULL;
    size_t len = 0;
    size_t read;

    while ((read = getline(&line, &len, stream)) != (size_t) -1) {
        input_logread_parse_line(line, entry);
        free(line);
        return lokicat_input_status_success;
    }

    free(line);
    return lokicat_input_status_eof;
}


lokicat_input_status input_logread20(char *buf, size_t *buf_len, Logproto__Entry *entry)
{
    char *line = NULL;
    size_t length = *buf_len;

    for (size_t i = 0; i < length; i++) {
        // new line, return entry
        if (buf[i] == '\n') {
            // copy over including new line
            line = malloc(i + 2);
            line[i + 1] = '\0';
            memcpy(line, buf, i + 1);

            // parse line
            input_logread_parse_line(line, entry);
            free(line);

            *buf_len = i;
            return lokicat_input_status_success;
        }
    }

    return lokicat_input_status_continue;
}
