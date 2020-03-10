bin_PROGRAMS = workifi
workifi_SOURCES = \
	src/workifi.h src/workifi.c \
	src/http.h src/http.c \
	src/utils.h src/utils.c \
	src/mime.h src/mime.c

workifi_CPPFLAGS = $(LIBCURL_CFLAGS)$(LIBJSONC_CFLAGS)
workifi_LDADD = $(LIBCURL_LIBS)$(LIBJSONC_LIBS)

if WINDOWS
LIBS += -lgdi32 -lcrypt32 -lz -lws2_32 -lssp
endif
