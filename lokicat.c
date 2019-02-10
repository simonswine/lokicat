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
    vfprintf (stderr, format, args);
    va_end (args);
    fprintf (stderr, "\n");
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
         "  --input=demo|logread Choose input, default is logread.\n"
        );
}

static protobuf_c_boolean starts_with (const char *str, const char *prefix)
{
    return memcmp (str, prefix, strlen (prefix)) == 0;
}

int main(int argc, char **argv)
{
    const char *loki_url = NULL;
    const char *input_name = NULL;

    for (unsigned i = 1; i < (unsigned) argc; i++) {
        if (starts_with (argv[i], "--url=")) {
            loki_url = strchr (argv[i], '=') + 1;
        } else if (starts_with (argv[i], "--input=")) {
            input_name = strchr (argv[i], '=') + 1;
        } else {
            usage ();
        }
    }

    if (loki_url == NULL) {
        die ("missing --url=URL");
    } else {
        char url[255];
        snprintf(url,255, "%s/api/prom/push", loki_url);
        loki_url = (const char *) &url;
    }

    if (input_name == NULL || (strcmp(input_name, "demo") != 0 && strcmp(input_name, "logread") != 0)) {
        die ("invalid or missing --input=");
    }

    fprintf(stderr, "input=%s loki_url=%s\n", input_name, loki_url);

    // build push request
    Logproto__PushRequest push_request = LOGPROTO__PUSH_REQUEST__INIT;
    void *buf;                     // buffer to store serialized data
    size_t len;                  // length of serialized data

    // allocate stream
    Logproto__Stream stream = LOGPROTO__STREAM__INIT;
    Logproto__Stream *streams [1];
    streams[0] = &stream;
    push_request.n_streams = 1;
    push_request.streams = streams;
    Logproto__Entry *entries [64];
    stream.entries = entries;

    // get hostname
    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);
    struct hostent* h;
    h = gethostbyname(hostname);

    // generate labels
    char labels[1024];
    labels[1023] = '\0';
    snprintf(labels, 1023, "{job=\"lokicat\",instance=\"%s\"}", h->h_name);
    stream.labels = labels;


    // gather log lines from stdin
    Logproto__Entry *entry = malloc(sizeof(Logproto__Entry));
    logproto__entry__init(entry);
    lokicat_input_status res;

    while (1) {

        res = input_logread(stdin, entry);

        if (res == lokicat_input_status_eof) {
            break;
        }

        if (res == lokicat_input_status_success) {
            fprintf(stderr,"retrieved entry of length %zu :\n", logproto__entry__get_packed_size(entry));
            stream.entries[stream.n_entries++] = entry;
            entry = malloc(sizeof(Logproto__Entry));
            logproto__entry__init(entry);
            continue;
        }

        printf("unknown result %d\n", res);

    }
    free(entry);

    // serialise
    len = logproto__push_request__get_packed_size(&push_request);  // This is calculated packing length
    buf = malloc (len);                      // Allocate required serialized buffer length
    logproto__push_request__pack (&push_request, buf);              // Pack the data

    // loop over streams
    for(size_t s = 0; s < push_request.n_streams; s++) {
        // loop over entries
        for(size_t e = 0; e < push_request.streams[s]->n_entries; e++) {
            Logproto__Entry *entry = push_request.streams[s]->entries[e];
            if (entry->timestamp != NULL) {
                free(entry->timestamp);
            }
            if (entry->line != NULL) {
                free(entry->line);
            }
            free(entry);
        }
    }

    // compress
    size_t compressed_len = snappy_max_compressed_length(len);      // length of compressed data
    void *compressed_buf = malloc (compressed_len);  // buffer to store compressed data
    int compressed_res = snappy_compress(buf, len, compressed_buf, &compressed_len);
    if (compressed_res != SNAPPY_OK) {
        die("error compressing: %d", compressed_res);
    }
    fprintf(stderr, "snappy compression bytes_before=%ld -> bytes_after=%ld\n", len, compressed_len); // See the length of message

    fprintf(stderr, "writing %ld serialized + compressed bytes\n", compressed_len); // See the length of message

    CURL *curl;
    CURLcode resp;
    struct curl_slist *list = NULL;

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl) {
        /* First set the URL that is about to receive our POST. This URL can
           just as well be a https:// URL if that is what should receive the
           data. */
        curl_easy_setopt(curl, CURLOPT_URL, loki_url);
        /* get verbose debug output please */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
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
        if (resp != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(resp));

        /* check response */
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code / 100 != 2) {
            die("unexpected response code from %s, %ld \n", loki_url, response_code);
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
        curl_slist_free_all(list);
    }
    curl_global_cleanup();


    free(buf);
    free(compressed_buf);


}
