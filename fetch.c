#!/usr/bin/env -S ccraft -p libcurl
// fetch - download URL to stdout
//
// Usage:
//   ./fetch URL
//
// Examples:
//   ./fetch https://example.com

#include <stdio.h>
#include <curl/curl.h>

int
main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: fetch URL\n");
        return 1;
    }
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return 0;
}
