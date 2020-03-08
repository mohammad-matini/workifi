#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <curl/curl.h>

#include "workifi.h"
#include "http.h"

#ifdef _WIN32
  #define LONG_FORMAT "I64d"
#else
  #define LONG_FORMAT "ld"
#endif


static int workifi_init (struct workifi_state *workifi);

static struct json_object *format_file_metadata(
        struct json_object *raw_file_metadata);

static int file_is_already_uploaded(
        struct array_list *file_list_array,
        const char *file_name);

int main ()
{
        struct workifi_state workifi;
        workifi_init(&workifi);

        printf("\nFound %"LONG_FORMAT" records to upload\n\n",
               array_list_length(workifi.file_list));

        if (!workifi_user_approves_config(&workifi)) {
                printf("okay, operation cancelled.\n");
                return EXIT_SUCCESS;
        }

        workifi_authenticate(&workifi);
        workifi_process_files(&workifi);

        curl_easy_cleanup(workifi.http_session);
        curl_global_cleanup();

        return EXIT_SUCCESS;
}


int workifi_user_approves_config (struct workifi_state *workifi)
{
        char response;

        printf("tenant: %s\n", (workifi->tenant));
        printf("username: %s\n", (workifi->username));
        printf("password: %s\n", (workifi->password));
        printf("\n");
        printf("list-id: %s\n", (workifi->list_id));
        printf("name-field-id: %s\n", (workifi->name_field_id));
        printf("file-field-id: %s\n", (workifi->file_field_id));
        printf("\n");
        printf("Please check the values above, "
               "then type \"yes\" to continue,\n"
               "or type \"no\" to cancel, then press enter.\n");
        printf("(yes/no) ? :: ");

        response = getchar();

        printf("\n");

        if (response == 'Y' || response == 'y') return 1;
        else return 0;
}


int workifi_parse_user_config (
        struct workifi_state *workifi,
        const char *config_path)
{
        struct json_object *config_json;
        config_json = workifi_load_json_file(config_path);

        workifi->tenant = json_object_get_string(
                json_object_object_get(config_json, "tenant"));
        workifi->username = json_object_get_string(
                json_object_object_get(config_json, "username"));
        workifi->password = json_object_get_string(
                json_object_object_get(config_json, "password"));

        workifi->list_id = json_object_get_string(
                json_object_object_get(config_json, "list-id"));
        workifi->name_field_id = json_object_get_string(
                json_object_object_get(config_json, "name-field-id"));
        workifi->file_field_id = json_object_get_string(
                json_object_object_get(config_json, "file-field-id"));

        return 0;
}


int workifi_parse_server_config (
        struct workifi_state *workifi,
        const char *config_path)
{
        struct json_object *config_json;
        config_json = workifi_load_json_file(config_path);

        workifi->api_endpoint_auth = json_object_get_string(
                json_object_object_get(config_json, "auth-url"));
        workifi->api_endpoint_data = json_object_get_string(
                json_object_object_get(config_json, "data-url"));
        workifi->api_endpoint_file = json_object_get_string(
                json_object_object_get(config_json, "file-url"));
        workifi->api_endpoint_update = json_object_get_string(
                json_object_object_get(config_json, "update-url"));

        return 0;
}



struct json_object *workifi_load_json_file (const char *file_path)
{
        struct json_object *parsed_json;

        FILE *file;
        long file_size;
        long file_size_read;
        char *buffer;


        file = fopen(file_path, "r");
        if (!file) goto failed_to_read_file;

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        if (file_size == -1) goto failed_to_read_file;
        rewind(file);

        buffer = calloc(file_size + 1, sizeof(char));
        file_size_read = fread(buffer, 1, file_size, file);
        if (file_size_read != file_size) goto failed_to_read_file;

        buffer[file_size] = 0;
        fclose(file);

        parsed_json = json_tokener_parse(buffer);
        free(buffer);

        return parsed_json;

failed_to_read_file: {
                fprintf(stderr, "Failed to read JSON file!\n");
                fprintf(stderr, "%s", file_path);
                exit(EXIT_FAILURE);
        }
}



int workifi_process_files(struct workifi_state *workifi)
{
        size_t i = 0;
        for (i = 0; i < array_list_length(workifi->file_list); i++) {
                struct json_object *record_definition =
                        array_list_get_idx(workifi->file_list, i);

                const char *record_name =
                        json_object_get_string(
                                json_object_object_get(
                                        record_definition, "record_name"));

                const char *file_name =
                        json_object_get_string(
                                json_object_object_get(
                                        record_definition, "file_path"));

                const char *file_path_prefix = "./files/";

                size_t file_path_size = (
                        strlen(file_path_prefix) +
                        strlen(file_name) + 1
                        );

                char *file_path = calloc(file_path_size, sizeof(char));

                snprintf(file_path, file_path_size, "%s%s",
                         file_path_prefix, file_name);

                printf("================================="
                       "===============================\n");

                printf("Record Number: %"LONG_FORMAT"\n"
                       "Record Name: %s\n"
                       "File Name: %s\n"
                       "Uploading...\n",
                       i, record_name, file_path);

                struct json_object *record =
                        workifi_find_record(workifi, record_name);

                if (!record) {
                        printf("ERROR: Record not found online!\n");
                        continue;
                }

                struct json_object *file_list = json_object_object_get(
                        record, workifi->file_field_id);

                if (!file_list) file_list = json_object_new_array();

                struct array_list *file_list_array =
                        json_object_get_array(file_list);

                if (file_is_already_uploaded(file_list_array, file_name)) {
                        printf("%s || File already uploaded\n", file_name);
                        continue;
                }

                struct json_object *uploaded_file_raw_metadata =
                        workifi_upload_file(workifi, file_path);

                struct json_object *uploaded_file_metadata =
                        format_file_metadata(uploaded_file_raw_metadata);

                struct json_object *record_update = json_object_new_object();

                json_object_array_add(file_list, uploaded_file_metadata);
                json_object_object_add(record_update, workifi->file_field_id,
                                       file_list);

                const char *record_id = json_object_get_string(
                        json_object_object_get(record, "_id"));

                workifi_update_record(workifi, record_id, record_update);

                free(file_path);
        }
        return 0;
}

static int workifi_init (struct workifi_state *workifi)
{
        workifi->server_url = "https://api.workiom.com";

        curl_global_init(CURL_GLOBAL_ALL);
        workifi->http_session = curl_easy_init();

        if (!workifi->http_session) {
                fprintf(stderr, "ERROR: libcurl initialization failure");
                exit(EXIT_FAILURE);
        }

        workifi_parse_server_config(workifi, WORKIFI_SERVER_CONFIG_PATH);
        workifi_parse_user_config(workifi, WORKIFI_USER_CONFIG_PATH);
        workifi->file_list = json_object_get_array(
                workifi_load_json_file(WORKIFI_FILE_LIST_PATH));

        return 0;
}

static struct json_object *format_file_metadata(
        struct json_object *raw_file_metadata)
{
        struct json_object *uploaded_file_metadata = json_object_new_object();

        json_object_object_add(
                uploaded_file_metadata, "_id",
                json_object_object_get(raw_file_metadata, "fileToken"));

        json_object_object_add(
                uploaded_file_metadata, "FileName",
                json_object_object_get(raw_file_metadata, "fileName"));

        json_object_object_add(
                uploaded_file_metadata, "HasThumbnail",
                json_object_object_get(raw_file_metadata, "hasThumbnail"));

        json_object_object_add(
                uploaded_file_metadata, "ContentType",
                json_object_object_get(raw_file_metadata, "fileType"));

        json_object_object_add(
                uploaded_file_metadata, "fileUrl",
                json_object_object_get(raw_file_metadata, "fileUrl"));

        json_object_object_add(
                uploaded_file_metadata, "thumbnailUrl",
                json_object_object_get(raw_file_metadata, "thumbnailUrl"));

        return uploaded_file_metadata;
}

static int file_is_already_uploaded(struct array_list *online_file_list,
                                    const char *file_name)
{
        for (size_t j = 0; j < array_list_length(online_file_list); j++) {
                const char *online_file_name =
                        json_object_get_string(
                                json_object_object_get(
                                        array_list_get_idx(
                                                online_file_list, j),
                                        "FileName"));
                if (strcmp(online_file_name, file_name) == 0) return 1;
        }

        return 0;
}
