#ifndef WORKIFI_UTILS_H
#define WORKIFI_UTILS_H

#include <sys/stat.h>

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#ifdef _WIN32
#define fopen _wfopen_hack
#define open _wopen_hack

__declspec( dllexport ) FILE *_wfopen_hack(const char *file, const char *mode);
__declspec( dllexport ) int _wopen_hack(const char *file, int oflags, ...);

#endif

#define WORKIFI_GREETING                                                \
        "\n=====================================================================\n" \
        "\n\nmm      mm                     mm           ##        mmmm      ##    \n" \
        "##      ##                     ##           \"\"       ##\"\"\"      \"\"    \n" \
        "\"#m ## m#\"  m####m    ##m####  ## m##\"    ####     #######    ####    \n" \
        " ## ## ##  ##\"  \"##   ##\"      ##m##        ##       ##         ##    \n" \
        " ###\"\"###  ##    ##   ##       ##\"##m       ##       ##         ##    \n" \
        " ###  ###  \"##mm##\"   ##       ##  \"#m   mmm##mmm    ##      mmm##mmm \n" \
        " \"\"\"  \"\"\"    \"\"\"\"     \"\"       \"\"   \""          \
        "\"\"  \"\"\"\"\"\"\"\"    \"\"      \"\"\"\"\"\"\"\" \n\n\n"       \
        "================ The Workiom Automated File Uploader ================\n\n\n"
#define WORKIFI_SEPERATOR \
        "=====================================================================\n"

struct workifi_string {
        char *ptr;
        size_t len;
};

struct workifi_file {
        FILE *file;
        int file_handle;
        const char *path;
        struct stat file_info;
};


void init_workifi_string(struct workifi_string *s);
void free_workifi_string(struct workifi_string *s);
struct workifi_string workifi_string_fmt(char const *fmt, ...);

int workifi_xferinfo_download_progress(
        void *p,
        curl_off_t dltotal,
        curl_off_t dlnow,
        curl_off_t ultotal,
        curl_off_t ulnow);

size_t workifi_curl_write_cb(
        void *ptr,
        size_t size,
        size_t nmemb,
        struct workifi_string *s);

size_t workifi_curl_file_read_cb(
        char *buffer,
        size_t size,
        size_t nmemb,
        void *workifi_file);

int workifi_curl_file_seek_cb(
        void *workifi_file,
        curl_off_t offset,
        int origin);

int initialize_logger ();

void writelog(char const *fmt, ...);

#endif  /* WORKIFI_UTILS_H */
