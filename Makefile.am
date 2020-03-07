bin_PROGRAMS = workifi
workifi_SOURCES = \
    src/workifi.h src/workifi.c \
    src/http.h src/http.c \
    src/utils.h src/utils.c

workifi_CPPFLAGS = $(LIBCURL_CFLAGS)$(LIBJSONC_CFLAGS)
workifi_LDADD = $(LIBCURL_LIBS)$(LIBJSONC_LIBS)

if MINGW
workifi_LDFLAGS = -Wl,--no-undefined,--enable-runtime-pseudo-reloc
LIBS += -lws2_32 -lz -lssp
endif
