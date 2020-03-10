#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <curl/curl.h>

#include "utils.h"


void init_workifi_string(struct workifi_string *s)
{
        s->len = 0;
        s->ptr = malloc(s->len+1);
        if (s->ptr == NULL) {
                fprintf(stderr, "malloc() failed\n");
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
                "\r [<o>    ]", "\r [ <o>   ]", "\r [  <o>  ]",
                "\r [   <o> ]", "\r [    <o>]", "\r [   <o> ]",
                "\r [  <o>  ]", "\r [ <o>   ]", "\r [<o>    ]",
        };

        fprintf(stderr, "%s", animation_frames[animation_loop_counter]);
        animation_loop_counter = animation_loop_counter == 8 ? 0 :
                animation_loop_counter + 1;

        fprintf(stderr,
                "  Progress: %"LONG_FORMAT"%%  :"
                ":  uploaded %"LONG_FORMAT" MiB out of "
                "%"LONG_FORMAT" MiB",
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
                fprintf(stderr, "realloc() failed\n");
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

#ifdef _WIN32
FILE *_wfopen_hack(const char *file, const char *mode)
{
	size_t wfile_size = MultiByteToWideChar(CP_UTF8, 0, file, -1, NULL, 0);
 	size_t wmode_size = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);

        wchar_t *wfile = calloc(wfile_size, sizeof(char));
        wchar_t *wmode = calloc(wmode_size, sizeof(char));

        MultiByteToWideChar(CP_UTF8, 0, file, strlen(file), wfile, wfile_size);
        MultiByteToWideChar(CP_UTF8, 0, mode, strlen(mode), wmode, wmode_size);

	wfile[wfile_size - 1] = '\0';
	wfile[wfile_size] = '\0';
	wmode[wmode_size - 1] = '\0';
	wmode[wmode_size] = '\0';

	FILE *handle;
	_wfopen_s(&handle, wfile, wmode);
	free(wfile);
	free(wmode);
        return handle;
}

int _wopen_hack(const char *file, int oflags, ...)
{
	size_t wfile_size = MultiByteToWideChar(CP_UTF8, 0, file, -1, NULL, 0);
        wchar_t *wfile = calloc(wfile_size, sizeof(char));
        int mode = 0;

        if(oflags & _O_CREAT)
        {
                va_list ap;
                va_start(ap, oflags);
                mode = (int)va_arg(ap, int);
                va_end(ap);
        }

        MultiByteToWideChar(CP_UTF8, 0, file, strlen(file), wfile, wfile_size);
	wfile[wfile_size - 1] = '\0';
	wfile[wfile_size] = '\0';

	int handle = _wopen(wfile, oflags, mode);
	free(wfile);
	return handle;
}
#endif
