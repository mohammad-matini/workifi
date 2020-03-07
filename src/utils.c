#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "utils.h"

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#ifdef _WIN32
  #define LONG_FORMAT "I64d"
#else
  #define LONG_FORMAT "ld"
#endif


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


size_t workifi_curl_write_callback(
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
