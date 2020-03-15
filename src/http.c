#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <json-c/json.h>
#include <curl/curl.h>

#include "workifi.h"
#include "http.h"
#include "utils.h"
#include "mime.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if WIN_32
#include <io.h>
#else
#include <unistd.h>
#endif

int workifi_authenticate(struct workifi_state *workifi)
{
        struct curl_slist *headers = NULL;

        const char *post_body_format;
        size_t post_body_size;
        char *post_body;

        struct workifi_response res;
        const char *bearer_prefix = "Authorization: Bearer ";
        const char *auth_token;

        size_t auth_header_size;
        char *auth_header;


        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");

        post_body_format = "{"
                "\"tenantName\": \"%s\","
                "\"userNameOrEmailAddress\": \"%s\","
                "\"password\": \"%s\","
                "\"twoFactorVerificationCode\": \"string\","
                "\"rememberClient\": true,"
                "\"twoFactorRememberClientToken\": \"string\","
                "\"singleSignIn\": true,"
                "\"returnUrl\": \"string\""
                "}";

        post_body_size = (
                strlen(post_body_format) +
                strlen(workifi->tenant) +
                strlen(workifi->username) +
                strlen(workifi->password) + 1
                );

        post_body = calloc(post_body_size, sizeof(char));

        snprintf(post_body, post_body_size, post_body_format,
                 workifi->tenant,
                 workifi->username,
                 workifi->password);

        workifi_http_post(workifi, &res, workifi->api_endpoint_auth,
                          headers, post_body);

        auth_token = json_object_get_string(
                json_object_object_get(
                        json_object_object_get(
                                res.body, "result"), "accessToken"));

        auth_header_size = strlen(bearer_prefix) + strlen(auth_token) + 1;

        auth_header = calloc(auth_header_size, sizeof(char));
        snprintf(auth_header, auth_header_size, "%s%s",
                 bearer_prefix, auth_token);

        workifi->auth_header = auth_header;

        curl_slist_free_all(headers);
        headers = NULL;
        free(post_body);
        return 0;
}


int workifi_update_record(struct workifi_state *workifi,
                          const char *id,
                          struct json_object *record_update)
{
        struct curl_slist *headers = NULL;

        struct workifi_response res;

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, workifi->auth_header);

        const char *url_format = "%s?listId=%s&id=%s";
        size_t url_size = (
                strlen(url_format) +
                strlen(workifi->api_endpoint_update) +
                strlen(workifi->list_id) +
                strlen(id) + 1
                );

        char *url = calloc(url_size, sizeof(char));


        snprintf(url, url_size, url_format,
                 workifi->api_endpoint_update, workifi->list_id, id);

        workifi_http_put(workifi, &res, url, headers,
                         json_object_get_string(record_update));

        free(url);
        return 0;
}


struct json_object *workifi_find_record(struct workifi_state *workifi,
                                        const char *record_name)
{
        struct curl_slist *headers = NULL;

        const char *post_body_format;
        size_t post_body_size;
        char *post_body;

        struct workifi_response res;

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, workifi->auth_header);

        post_body_format = "{"
                "\"listId\":\"%s\","
                "\"filter\":[{\"fieldId\":%s,\"operator\":3,\"value\":\"%s\"}],"
                "\"linkedItemsLimit\":null,"
                "\"projectedFields\":[%s,%s],"
                "\"sorting\":\"\","
                "\"maxResultCount\":1,"
                "\"skipCount\":0"
                "}";

        post_body_size = (
                strlen(post_body_format) +
                strlen(workifi->list_id) +
                strlen(workifi->name_field_id) +
                strlen(record_name) +
                strlen(workifi->name_field_id) +
                strlen(workifi->file_field_id) + 1
                );

        post_body = calloc(post_body_size, sizeof(char));

        snprintf(post_body, post_body_size, post_body_format,
                 workifi->list_id,
                 workifi->name_field_id, record_name,
                 workifi->name_field_id, workifi->file_field_id);

        workifi_http_post(workifi, &res, workifi->api_endpoint_data,
                          headers, post_body);

        curl_slist_free_all(headers);
        free(post_body);

        return array_list_get_idx(
                json_object_get_array(
                        json_object_object_get(
                                json_object_object_get(
                                        res.body, "result"), "items")), 0);
}


struct json_object *workifi_upload_file(struct workifi_state *workifi,
                                        const char *file_path)
{
        struct curl_slist *headers = NULL;
        struct workifi_response res;
        struct workifi_file file;

        file.path = file_path;
        file.file = fopen(file.path, "rb");
        if (!file.file) return NULL;

        file.file_handle = fileno(file.file);
        fstat(file.file_handle, &file.file_info);

        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, workifi->auth_header);

        workifi_http_post_file(workifi, &res,
                               workifi->api_endpoint_file,
                               headers, &file);

        curl_slist_free_all(headers);
        fclose(file.file);
        headers = NULL;

        return array_list_get_idx(
                json_object_get_array(
                        json_object_object_get(
                                res.body, "result")), 0);
}


int workifi_http_post(
        struct workifi_state *workifi,
        struct workifi_response *res,
        const char *url,
        struct curl_slist *headers,
        const char *body)
{
        CURL *curl = workifi->http_session;
        CURLcode curl_code;

        struct workifi_string response_body;

        init_workifi_string(&response_body);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         workifi_curl_write_cb);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_code = curl_easy_perform(curl);

        if(curl_code != CURLE_OK) goto connection_failed;

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) goto request_error;

        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &res->status);
        res->body = json_tokener_parse(response_body.ptr);

        free_workifi_string(&response_body);
        curl_easy_reset(curl);
        return (int)curl_code;

connection_failed: {
                writelog("find record: connection failed: %s\n",
                         curl_easy_strerror(curl_code));
                exit(EXIT_FAILURE);
        }
request_error: {
                writelog("find record: request failed \n "
                         "request-payload: %s\n status-code: %ld \n "
                         "response-body: %s\n",
                         body, response_code, response_body.ptr);
                exit(EXIT_FAILURE);
        }
}


int workifi_http_post_file(
        struct workifi_state *workifi,
        struct workifi_response *res,
        const char *url,
        struct curl_slist *headers,
        struct workifi_file *file)
{
        CURLcode curl_code;
        CURL *curl = workifi->http_session;
        curl_mime *mime;
        curl_mimepart *part;

        struct workifi_string response_body;

        init_workifi_string(&response_body);

        mime = NULL;
        mime = curl_mime_init(curl);

        part = curl_mime_addpart(mime);

        curl_mime_data_cb(
                part, (curl_off_t)file->file_info.st_size,
                workifi_curl_file_read_cb,
                workifi_curl_file_seek_cb,
                NULL, file);

        curl_mime_name(part, "upload");
        curl_mime_filename(part, basename( (char *) file->path));
        curl_mime_type(part, workifi_mime_content_type(file->path));

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         workifi_curl_write_cb);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_URL, url);

        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                         (curl_off_t)file->file_info.st_size);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
                         workifi_xferinfo_download_progress);

        curl_code = curl_easy_perform(curl);

        if(curl_code != CURLE_OK) goto connection_failed;

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) goto request_error;

        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &res->status);
        res->body = json_tokener_parse(response_body.ptr);

        free_workifi_string(&response_body);

        curl_mime_free(mime);
        mime = NULL;
        curl_easy_reset(curl);

        return (int)curl_code;

connection_failed: {
                writelog("file upload: connection failed: %s\n",
                        curl_easy_strerror(curl_code));
                exit(EXIT_FAILURE);
        }
request_error: {
                writelog("file upload: request failed \n "
                         "status-code: %ld \n "
                         "response-body: %s\n",
                         response_code, response_body.ptr);
                exit(EXIT_FAILURE);
        }
}


int workifi_http_put(struct workifi_state *workifi,
                     struct workifi_response *res,
                     const char *url,
                     struct curl_slist *headers,
                     const char *body)
{
        CURL *curl = workifi->http_session;
        CURLcode curl_code;

        struct workifi_string response_body;

        init_workifi_string(&response_body);

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         workifi_curl_write_cb);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_code = curl_easy_perform(curl);

        if (curl_code != CURLE_OK) goto connection_failed;

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) goto request_error;

        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &res->status);
        res->body = json_tokener_parse(response_body.ptr);

        free_workifi_string(&response_body);
        curl_easy_reset(curl);
        return (int)curl_code;

connection_failed: {
                writelog("record update: connection failed: %s\n",
                        curl_easy_strerror(curl_code));
                exit(EXIT_FAILURE);
        }
request_error: {
                writelog("record update: request failed \n "
                         "request-payload: %s\n status-code: %ld \n "
                         "response-body: %s\n",
                         body, response_code, response_body.ptr);
                exit(EXIT_FAILURE);
        }
}
