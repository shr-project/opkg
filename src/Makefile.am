AM_CFLAGS = -I${top_srcdir}/libopkg ${ALL_CFLAGS}
bin_PROGRAMS = opkg-cl opkg-key

opkg_cl_SOURCES = opkg-frontend.c
opkg_cl_LDADD = $(top_builddir)/libopkg/libopkg.la \
                $(top_builddir)/libbb/libbb.la 

opkg_key_SOURCES = opkg-key.c
opkg_key_LDADD = $(GPGME_LIBS) 
