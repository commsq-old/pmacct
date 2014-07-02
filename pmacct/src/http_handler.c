#ifndef __HTTP_HANDLER_C
#define __HTTP_HANDLER_C

#include <string.h>
#include <errno.h>

#include "http_handler.h"

#define HTTP_STRING "HTTP/"

/* Parse a HTTP client request; bail at first sign of an invalid request */
int parse_client_request(char *header_line)
{
    char *method, *request_uri, *http_version;

    method = header_line;

    if ((request_uri = strchr(method, ' ')) == NULL) return 1;
    while (isspace(*request_uri)) request_uri++;

    if ((http_version = strchr(request_uri, ' ')) != NULL) {
        while (isspace(*http_version)) http_version++;
        if (strncmp(http_version, HTTP_STRING, strlen(HTTP_STRING)) != 0) return 1;
    }

    return 0;
}

/*
 * Parse the HTTP GET request in a received frame to find the host header field
 * and compare its value against a given one
 */
char *locate_http_host(char *buf)
{
    static const char *host_header_name = "Host";

    char *header_line, *req_value;

    // Parse header line, bail if malformed
    if ((header_line = strtok(buf, "\n")) == NULL) return NULL;

    if (parse_client_request(header_line)) return NULL;

    // Iterate through request/entity header fields
    while ((header_line = strtok(NULL, "\n")) != NULL) {
        if ((req_value = strchr(header_line, ':')) == NULL) continue;
        req_value++;

        // Identify the "Host" header
        if (strncmp(header_line, host_header_name, strlen(host_header_name)) == 0) {

            while (isspace(*req_value)) req_value++;

            // When using strtok remove trailing '\r' if any
            if (*(req_value + strlen(req_value) - 1) == '\r')
                *(req_value + strlen(req_value) - 1) = '\0';

            return req_value;
        }
    }

    return NULL;
}

#endif // __HTTP_HANDLER_C
