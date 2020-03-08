#ifndef WORKIFI__H
#define WORKIFI_H

#define WORKIFI_SERVER_CONFIG_PATH "urls.json"
#define WORKIFI_USER_CONFIG_PATH "config.json"
#define WORKIFI_FILE_LIST_PATH "list.json"

struct workifi_state {
        const char *server_url;
        const char *api_endpoint_auth;
        const char *api_endpoint_data;
        const char *api_endpoint_file;
        const char *api_endpoint_update;

        const char *tenant;
        const char *username;
        const char *password;

        const char *list_id;
        const char *name_field_id;
        const char *file_field_id;

        CURL *http_session;
        const char *auth_header;
        struct curl_slist *http_headers;

        struct array_list *file_list;
};


struct json_object *workifi_load_json_file (const char *file_path);

int workifi_user_approves_config(struct workifi_state *config);
int workifi_parse_file_list (const char *file_list_path);
int workifi_process_files(struct workifi_state *workifi);

int workifi_parse_user_config (
        struct workifi_state *workifi,
        const char *config_path);
int workifi_parse_server_config (
        struct workifi_state *workifi,
        const char *config_path);

#endif /* WORKIFI__H */
