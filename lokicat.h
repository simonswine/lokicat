#ifndef LOKICAT_H
#define LOKICAT_H

#define LOG_USE_COLOR

#include "log.h"

// return values for input parser
typedef enum {
    lokicat_input_status_success, // finished parsing single entry
    lokicat_input_status_eof, // need more input
    lokicat_input_status_continue, // need more input
    lokicat_input_status_error, // error happened
} lokicat_input_status;

#endif  /* LOKICAT_H */
