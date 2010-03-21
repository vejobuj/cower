/*
 *  curlhelper.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

#include <jansson.h>

#include "json.h"
#include "aur.h"

static size_t write_response(void *ptr, size_t size, size_t nmemb, void *stream) {
    struct write_result *result = (struct write_result *)stream;

    if(result->pos + size * nmemb >= JSON_BUFFER_SIZE - 1) {
        fprintf(stderr, "error: too small buffer\n");
        return 0;
    }

    memcpy(result->data + result->pos, ptr, size * nmemb);
    result->pos += size * nmemb;

    return size * nmemb;
}

char *curl_get_json(int type, const char *arg) {
    CURL *curl;
    CURLcode status;
    char *data;
    char url[AUR_RPC_URL_SIZE];
    long code;

    curl = curl_easy_init();
    data = malloc(JSON_BUFFER_SIZE);
    if(!curl || !data) return NULL;

    struct write_result write_result = {
        .data = data,
        .pos = 0
    };

    snprintf(url, AUR_RPC_URL_SIZE, AUR_RPC_URL,
        type == AUR_RPC_QUERY_TYPE_INFO ? "info" : "search", 
        arg);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);

    status = curl_easy_perform(curl);
    if(status != 0) {
        fprintf(stderr, "error: unable to request data from %s:\n", url);
        fprintf(stderr, "%s\n", curl_easy_strerror(status));
        return NULL;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    if(code != 200) {
        fprintf(stderr, "error: server responded with code %ld\n", code);
        return NULL;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    /* zero-terminate the result */
    data[write_result.pos] = '\0';

    return data;
}

/*
int main(int argc, char *argv[]) {
    char *result = curl_get_json(atoi(argv[1]), argv[2]);
    json_t *root;
    json_error_t error;

    if (! result) printf("result is NULL!\n");

    root = json_loads(result, &error);

    free(result);
    json_decref(root);

    return 0;
}
*/