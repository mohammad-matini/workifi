#ifndef WORKIFI_HTTP_H
#define WORKIFI_HTTP_H

struct workifi_response {
        long status;
        struct json_object *body;
};


int workifi_authenticate(
        struct workifi_state *workifi);

int workifi_update_record(
        struct workifi_state *workifi,
        const char *id,
        struct json_object *record_update);

struct json_object *workifi_find_record(
        struct workifi_state *workifi,
        const char *record_name);

struct json_object *workifi_upload_file(
        struct workifi_state *workifi,
        const char *file_path);

int workifi_http_post(
        struct workifi_state *workifi,
        struct workifi_response *res,
        const char *url,
        struct curl_slist *headers,
        const char *body);

int workifi_http_post_file(
        struct workifi_state *workifi,
        struct workifi_response *res,
        const char *url,
        struct curl_slist *headers,
        const char *file_path);

int workifi_http_put(
        struct workifi_state *workifi,
        struct workifi_response *res,
        const char *url,
        struct curl_slist *headers,
        const char *body);

#endif  /* WORKIFI_HTTP_H */
