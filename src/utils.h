#ifndef WORKIFI_UTILS_H
#define WORKIFI_UTILS_H

struct workifi_string {
        char *ptr;
        size_t len;
};


void init_workifi_string(struct workifi_string *s);
void free_workifi_string(struct workifi_string *s);
 
int workifi_xferinfo_download_progress(
        void *p,
        curl_off_t dltotal,
        curl_off_t dlnow,
        curl_off_t ultotal,
        curl_off_t ulnow);

size_t workifi_curl_write_callback(
        void *ptr,
        size_t size,
        size_t nmemb,
        struct workifi_string *s);

#endif  /* WORKIFI_UTILS_H */
