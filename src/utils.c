#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <curl/curl.h>

#include "utils.h"

static void workifi_greeting() {
        writelog(WORKIFI_GREETING);
}

void init_workifi_string(struct workifi_string *s)
{
        s->len = 0;
        s->ptr = malloc(s->len+1);
        if (s->ptr == NULL) {
                writelog("malloc() failed\n");
                exit(EXIT_FAILURE);
        }
        s->ptr[0] = '\0';
}


void free_workifi_string(struct workifi_string *s)
{
        free(s->ptr);
}


int workifi_xferinfo_download_progress(
        void *UNUSED(p),
        curl_off_t UNUSED(dltotal),
        curl_off_t UNUSED(dlnow),
        curl_off_t ultotal,
        curl_off_t ulnow)
{
        static int animation_loop_counter = 0;
        const char *animation_frames[] = {
                "[<o>    ]", "[ <o>   ]", "[  <o>  ]",
                "[   <o> ]", "[    <o>]", "[   <o> ]",
                "[  <o>  ]", "[ <o>   ]", "[<o>    ]",
        };

        fprintf(stderr, "%s", animation_frames[animation_loop_counter]);
        animation_loop_counter = animation_loop_counter == 8 ? 0 :
                animation_loop_counter + 1;

        fprintf(stderr,
                "  Progress: %"LONG_FORMAT"%%  :"
                ":  uploaded %"LONG_FORMAT" MiB out of "
                "%"LONG_FORMAT" MiB\r",
                ultotal <= 0 ? 0 : ((ulnow * 100) / ultotal),
                (ulnow / (1024*1024)), ultotal / (1024*1024));

        return 0;
}


size_t workifi_curl_write_cb(
        void *ptr,
        size_t size,
        size_t nmemb,
        struct workifi_string *s)
{
        size_t new_len = s->len + size*nmemb;
        s->ptr = realloc(s->ptr, new_len+1);

        if (s->ptr == NULL) {
                writelog("realloc() failed\n");
                exit(EXIT_FAILURE);
        }
        memcpy(s->ptr+s->len, ptr, size*nmemb);
        s->ptr[new_len] = '\0';
        s->len = new_len;

        return size*nmemb;
}


size_t workifi_curl_file_read_cb(
        char *buffer,
        size_t size,
        size_t nmemb,
        void *workifi_file)
{
        struct workifi_file *file = (struct workifi_file *) workifi_file;
        return fread(buffer, size, nmemb, file->file);
}

int workifi_curl_file_seek_cb(
        void *workifi_file,
        curl_off_t offset,
        int origin)
{
        struct workifi_file *file = (struct workifi_file *) workifi_file;
        return fseek(file->file, offset, origin) ?
                CURL_SEEKFUNC_OK : CURL_SEEKFUNC_FAIL;
}

static FILE *log_file;

int initialize_logger () {
        const char *file_path_prefix = "./logs/";
        const char *file_path_postfix = ".log";
        const char *iso_datetime_example = "2020-03-11T02:07:27";

        size_t file_path_size = (
                strlen(file_path_prefix) +
                strlen(iso_datetime_example) +
                strlen(file_path_postfix)
                );

        char *current_datetime_iso_string =
		calloc(strlen(iso_datetime_example) + 1, sizeof(char));
        char *file_path = calloc(file_path_size + 1, sizeof(char));

        time_t now;
        time(&now);

        strftime(current_datetime_iso_string,
                 strlen(iso_datetime_example) + 1,
                 "%Y-%m-%d-%H-%M-%S", localtime(&now));

        snprintf(file_path, file_path_size + 1, "%s%s%s",
                 file_path_prefix,
                 current_datetime_iso_string ,
                 file_path_postfix);

        struct stat st = {0};
        if (stat("./logs", &st) == -1) {
                #ifdef _WIN32
                mkdir("./logs");
                #else
                mkdir("./logs", 0700);
                #endif
        }

        log_file = fopen(file_path, "a+");

        if (!log_file) {
                printf("ERROR: could not initialize log file: %s\n", file_path);
                exit(EXIT_FAILURE);
        }

        workifi_greeting();
        writelog("Logging progress to %s\n", file_path);

        return 0;
}

void writelog(char const *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
        va_start(ap, fmt);
        vfprintf(log_file, fmt, ap);
        fflush(log_file);
        va_end(ap);
}

#ifdef _WIN32
FILE *_wfopen_hack(const char *file, const char *mode)
{
        size_t wfile_size = MultiByteToWideChar(CP_UTF8, 0, file, -1, NULL, 0);
        size_t wmode_size = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);

        wchar_t *wfile = calloc(wfile_size, sizeof(wchar_t));
        wchar_t *wmode = calloc(wmode_size, sizeof(wchar_t));

        MultiByteToWideChar(CP_UTF8, 0, file, -1, wfile, wfile_size);
        MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, wmode_size);

        FILE *handle;
        _wfopen_s(&handle, wfile, wmode);
        free(wfile);
        free(wmode);
        return handle;
}

int _wopen_hack(const char *file, int oflags, ...)
{
        size_t wfile_size = MultiByteToWideChar(CP_UTF8, 0, file, -1, NULL, 0);
        wchar_t *wfile = calloc(wfile_size, sizeof(wchar_t));
        int mode = 0;

        if(oflags & _O_CREAT)
        {
                va_list ap;
                va_start(ap, oflags);
                mode = (int)va_arg(ap, int);
                va_end(ap);
        }

        MultiByteToWideChar(CP_UTF8, 0, file, -1, wfile, wfile_size);

        int handle = _wopen(wfile, oflags, mode);
        free(wfile);
        return handle;
}
#endif
