#ifndef WORKIFI_MIME_H
#define WORKIFI_MIME_H
#include <sys/stat.h>

struct workifi_content_type {
        const char *extension;
        const char *type;
};


const char *workifi_mime_content_type(const char *filename);

#endif  /* WORKIFI_MIME_H */

