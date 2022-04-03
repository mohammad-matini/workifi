#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <json-c/json.h>
#include <curl/curl.h>

#include "workifi.h"
#include "http.h"
#include "utils.h"

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

        writelog("\nFound %"PRIi64" records to upload\n\n",
               array_list_length(workifi.file_list));

        if (!workifi_user_approves_config(&workifi)) {
                writelog("okay, operation cancelled.\n");
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

        writelog("tenant: %s\n", workifi->tenant);
        writelog("username: %s\n", workifi->username);
        writelog("password: %s\n", workifi->password);
        writelog("\n");
        writelog("list-id: %s\n", workifi->list_id);
        writelog("name-field-id: %s\n", workifi->name_field_id);
        writelog("file-field-id: %s\n", workifi->file_field_id);
        writelog("\n");

        if (!(workifi->tenant && strlen(workifi->tenant) > 0 &&
              workifi->username && strlen(workifi->username) > 0 &&
              workifi->password && strlen(workifi->password) > 0 && 
	      workifi->list_id && strlen(workifi->list_id) > 0 &&
              workifi->name_field_id && strlen(workifi->name_field_id) > 0 && 
	      workifi->file_field_id && strlen(workifi->file_field_id) > 0)) {
                writelog("ERROR: Missing required configurations, "
                         "please check the values above\nand fix them "
                         "in config.json, then try again.\n");
                exit(EXIT_FAILURE);
        }

        writelog("Please check the values above, "
               "then type \"yes\" to continue,\n"
               "or type \"no\" to cancel, then press enter.\n");
        writelog("(yes/no) ? :: ");

        response = getchar();

        writelog("\n");

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

        workifi->server_url = json_object_get_string(
                json_object_object_get(config_json, "base-url"));
        workifi->api_endpoint_auth = json_object_get_string(
                json_object_object_get(config_json, "auth-url"));
        workifi->api_endpoint_data = json_object_get_string(
                json_object_object_get(config_json, "data-url"));
        workifi->api_endpoint_file = json_object_get_string(
                json_object_object_get(config_json, "file-url"));
        workifi->api_endpoint_update = json_object_get_string(
                json_object_object_get(config_json, "update-url"));
        workifi->api_endpoint_get_upload_url = json_object_get_string(
                json_object_object_get(config_json, "get-upload-url"));
        workifi->api_endpoint_confirm_upload = json_object_get_string(
                json_object_object_get(config_json, "confirm-upload-url"));

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
                writelog("Failed to read JSON file!\n");
                writelog("%s\n", file_path);
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

                writelog(WORKIFI_SEPERATOR);

                writelog("Record Number: %"PRIi64"\n"
                       "Record Name: %s\n"
                       "File Name: %s\n"
                       "Uploading...\n",
                       i, record_name, file_path);

                struct json_object *record =
                        workifi_find_record(workifi, record_name);

                if (!record) {
                        writelog("ERROR: Record not found online!\n");
                        free(file_path);
                        continue;
                }

                struct json_object *file_list = json_object_object_get(
                        record, workifi->file_field_id);

                if (!file_list) file_list = json_object_new_array();

                struct array_list *file_list_array =
                        json_object_get_array(file_list);

                if (file_is_already_uploaded(file_list_array, file_name)) {
                        writelog("File already uploaded.\n", file_name);
                        free(file_path);
                        continue;
                }

                struct json_object *upload_url_metadata =
                    workifi_get_file_upload_url(workifi, file_name);

                const char *upload_url = json_object_get_string(
                        json_object_object_get(upload_url_metadata,
                            "fileUrl"));

                const char *file_token = json_object_get_string(
                        json_object_object_get(upload_url_metadata,
                            "fileToken"));

                if (workifi_upload_file(workifi, file_path, upload_url) < 0) {
                        writelog("ERROR: Could not read file for upload! "
                                 "Check the file name, and make\nsure that it "
                                 "can be opened.\n",
                                 file_path);
                        free(file_path);
                        continue;
                }

                struct json_object *uploaded_file_raw_metadata =
                        workifi_confirm_file_upload(workifi, file_token);

                struct json_object *uploaded_file_metadata =
                        format_file_metadata(uploaded_file_raw_metadata);

                struct json_object *record_update = json_object_new_object();

                json_object_array_add(file_list, uploaded_file_metadata);
                json_object_object_add(record_update, workifi->file_field_id,
                                       file_list);

                const char *record_id = json_object_get_string(
                        json_object_object_get(record, "_id"));

                workifi_update_record(workifi, record_id, record_update);
                writelog("File uploaded successfully\n");

                free(file_path);
        }
        return 0;
}

static int workifi_init (struct workifi_state *workifi)
{
        initialize_logger();

        curl_global_init(CURL_GLOBAL_ALL);
        workifi->http_session = curl_easy_init();

        if (!workifi->http_session) {
                writelog("ERROR: libcurl initialization failure\n");
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
