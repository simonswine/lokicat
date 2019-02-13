#define _GNU_SOURCE
#define __USE_XOPEN_EXTENDED
#include <ctype.h>
#include <curl/curl.h>
#include <errno.h>
#include <snappy-c.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>

#include "lokicat.h"
#include "input_logread.h"
#include "logproto/logproto.pb-c.h"

static void die (const char *format, ...)
{
    va_list args;
    va_start (args, format);
    log_fatal(format, args);
    va_end (args);
    exit (1);
}

static void usage (void)
{
    die ("usage: lokicat --url=URL\n"
         "\n"
         "Run a loki forwarder"
         "\n"
         "Options:\n"
         "  --url=URL  URL for the loki API.\n"
         "  --input=logread Choose input, default is logread.\n"
        );
}

static protobuf_c_boolean starts_with (const char *str, const char *prefix)
{
    return memcmp (str, prefix, strlen (prefix)) == 0;
}

int send_push_request(Logproto__PushRequest *push_request, const char *loki_url)
{
    void *buf;           // buffer to store serialized data
    size_t len;          // length of serialized data
    int return_code = 0; // return code, 0 == fine, 1 == failed

    // serialise
    len = logproto__push_request__get_packed_size(push_request);  // This is calculated packing length
    buf = malloc (len);                      // Allocate required serialized buffer length
    logproto__push_request__pack (push_request, buf);              // Pack the data

    // compress
    size_t compressed_len = snappy_max_compressed_length(len);      // length of compressed data
    void *compressed_buf = malloc (compressed_len);  // buffer to store compressed data
    int compressed_res = snappy_compress(buf, len, compressed_buf, &compressed_len);
    if (compressed_res != SNAPPY_OK) {
        log_error("error compressing: %d", compressed_res);
        free(compressed_buf);
        free(buf);
        return 1;
    }

    log_debug("snappy compression bytes_before=%zu bytes_after=%zu", len, compressed_len);


    CURL *curl;
    CURLcode resp;
    struct curl_slist *list = NULL;

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if (!curl) {
        log_fatal("error intialising curl");
    }

    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */
    curl_easy_setopt(curl, CURLOPT_URL, loki_url);
    /* get verbose debug output please */
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    //
    /* Now specify the POST data */

    /* Set user agent header */
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "lokicat/0.1.0");

    /* Declare we are using protobuf */
    list = curl_slist_append(list, "Content-Type: application/octet-stream");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    /* size of the POST data */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, compressed_len);

    /* pass in a pointer to the data - libcurl will not copy */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, compressed_buf);

    /* Perform the request, resp will get the return code */
    resp = curl_easy_perform(curl);
    /* Check for errors */
    if (resp != CURLE_OK) {
        log_fatal("curl_easy_perform() failed: %s", curl_easy_strerror(resp));
        return_code = 0;
    }

    /* check response */
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code / 100 != 2) {
        log_warn("unexpected response code from %s, %ld", loki_url, response_code);
        return_code = 0;
    } else {
        // count the lines
        size_t lines = 0;
        for (size_t s = 0; s < push_request->n_streams; s++) {
            lines += push_request->streams[s]->n_entries;
        }
        log_info("sent bytes=%zu lines=%zu to loki", lines, compressed_len);
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_slist_free_all(list);

    curl_global_cleanup();

    free(buf);
    free(compressed_buf);

    return return_code;
}

// resets all entries in the push request
void push_request_reset(Logproto__PushRequest *push_request)
{
    // loop over streams
    for (size_t s = 0; s < push_request->n_streams; s++) {
        // loop over entries
        for (size_t e = 0; e < push_request->streams[s]->n_entries; e++) {
            Logproto__Entry *entry = push_request->streams[s]->entries[e];
            if (entry->timestamp != NULL) {
                free(entry->timestamp);
            }
            if (entry->line != NULL) {
                free(entry->line);
            }
            free(entry);
        }
        push_request->streams[s]->n_entries = 0;
    }


}

// handle log send events
void flush_push_request(Logproto__PushRequest *push_request, const char *loki_url, int *timestamp_oldest_entry)
{
    send_push_request(push_request, loki_url);
    push_request_reset(push_request);
    *timestamp_oldest_entry = 0;
}

int main(int argc, char **argv)
{
    const char *loki_url = NULL;
    const char *input_name = "logread";

    // how many entries to buffer locally
    unsigned int max_entries = 64;
    // how long should we wait to flush a log entry
    unsigned int max_age_of_entry = 30;

    for (unsigned i = 1; i < (unsigned) argc; i++) {
        if (starts_with (argv[i], "--url=")) {
            loki_url = strchr (argv[i], '=') + 1;
        } else {
            usage ();
        }
    }

    if (loki_url == NULL) {
        die ("missing --url=URL");
    } else {
        char url[255];
        snprintf(url, 255, "%s/api/prom/push", loki_url);
        loki_url = (const char *) &url;
    }

    if (input_name == NULL || (strcmp(input_name, "demo") != 0 && strcmp(input_name, "logread") != 0)) {
        die ("invalid or missing --input=");
    }

    log_debug("input = %s", input_name);
    log_debug("loki_url = %s", loki_url);
    log_debug("max_entries = %u", max_entries);

    // build push request
    Logproto__PushRequest push_request = LOGPROTO__PUSH_REQUEST__INIT;

    // allocate stream
    Logproto__Stream stream = LOGPROTO__STREAM__INIT;
    Logproto__Stream *streams [1];
    streams[0] = &stream;
    push_request.n_streams = 1;
    push_request.streams = streams;
    Logproto__Entry *entries [max_entries];
    stream.entries = entries;

    // get hostname
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    struct hostent *h;
    h = gethostbyname(hostname);

    // generate labels
    char labels[1024];
    labels[1023] = '\0';
    snprintf(labels, 1023, "{job=\"lokicat\",instance=\"%s\"}", h->h_name);
    stream.labels = labels;

    // timeout value for reading 500ms
    struct timeval timeout;

    // file descriptor to read from
    int fd = fileno(stdin);

    // result for select
    int res_select = 0;

    // allocate entry
    Logproto__Entry *entry = malloc(sizeof(Logproto__Entry));
    logproto__entry__init(entry);

    // result for input method
    lokicat_input_status res_input;

    // read buffer
    size_t read_buf_len = 2;
    ssize_t read_len = 0;
    char read_buf[read_buf_len];

    // configure fd set
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);

    int timestamp_oldest_entry = 0;

    // main loop
    while (1) {

        if (timestamp_oldest_entry > 0 && ((timestamp_oldest_entry + (int)max_age_of_entry) < (int)time(NULL))) {
            flush_push_request(&push_request, loki_url, &timestamp_oldest_entry);
        }

        // set timeout values
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        // wait for available data
        res_select = select(fd + 1, &set, NULL, NULL, &timeout);
        if (res_select == -1) {
            die("select unexpected error");    /* an error accured */
        } else if (res_select == 0) {
        } else {
            read_len = read(fd, &read_buf, read_buf_len); /* there was data to read */
            if (read_len < 0) {
                die("error during read: %s", strerror(errno));
            } else if (read_len == 0) {
                // EOF
                break;
            }

            // forward read to the input
            char *start = read_buf;
            char *end = read_buf + read_len - 1;
            while (start <= end) {
                size_t len = end - start + 1;
                res_input = input_logread20(start, &len, entry);

                if (res_input == lokicat_input_status_continue) {
                    break; // need more input, read again
                }

                if (res_input == lokicat_input_status_success) {
                    log_debug("retrieved protobuf log_entry of length=%zu", logproto__entry__get_packed_size(entry));

                    // TODO: ensure enough space in buffer
                    if (timestamp_oldest_entry == 0) {
                        timestamp_oldest_entry = (int) time(NULL);
                    }
                    stream.entries[stream.n_entries++] = entry;

                    if (stream.n_entries >= max_entries) {
                        flush_push_request(&push_request, loki_url, &timestamp_oldest_entry);
                    }

                    entry = malloc(sizeof(Logproto__Entry));
                    logproto__entry__init(entry);

                    start += len;


                }
            }
        }
    }

    /*


        res = input_logread(stdin, entry);

        if (res == lokicat_input_status_eof) {
            break;
        }

        if (res == lokicat_input_status_success) {
            fprintf(stderr, "retrieved entry of length %zu :\n", logproto__entry__get_packed_size(entry));
            stream.entries[stream.n_entries++] = entry;
            entry = malloc(sizeof(Logproto__Entry));
            logproto__entry__init(entry);
            continue;
        }

        printf("unknown result %d\n", res);

    }
    free(entry);

    */

}
