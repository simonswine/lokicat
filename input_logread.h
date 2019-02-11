#ifndef LOKICAT_INPUT_LOGREAD_H
#define LOKICAT_INPUT_LOGREAD_H

#include "lokicat.h"
#include "logproto/logproto.pb-c.h"

lokicat_input_status input_logread(FILE *stream, Logproto__Entry *entry);

// take a logread line and parse it to entry
void input_logread_parse_line(char *line, Logproto__Entry *entry);

#endif  /* LOKICAT_INPUT_LOGREAD_H */
