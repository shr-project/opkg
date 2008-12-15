#SUBDIRS = etc replace familiar libbb
SUBDIRS = etc familiar libbb

HOST_CPU=@host_cpu@
BUILD_CPU=@build_cpu@
OPKGLIBDIR=@opkglibdir@ 
ALL_CFLAGS=-g -O -Wall -DHOST_CPU_STR=\"@host_cpu@\" -DBUILD_CPU=@build_cpu@ -DLIBDIR=\"@libdir@\" -DOPKGLIBDIR=\"@opkglibdir@\" -DDATADIR=\"@datadir@\"

bin_PROGRAMS = opkg-cl

lib_LTLIBRARIES = libopkg.la

bin_SCRIPTS = update-alternatives

interceptdir = $(datadir)/opkg/intercept
intercept_DATA = intercept/ldconfig intercept/depmod intercept/update-modules

install-data-hook:
	chmod +x $(DESTDIR)$(datadir)/opkg/intercept/*

# opkg_LDADD = libbb/libbb.a replace/libreplace.a

#opkg_cl_LDADD = libopkg.la libbb/libbb.la replace/libreplace.a 
opkg_cl_LDADD = libopkg.la libbb/libbb.la 

libopkg_includedir=$(includedir)/libopkg

libopkg_include_HEADERS= \
	args.h \
	conffile.h \
	conffile_list.h \
	config.h \
	hash_table.h \
	includes.h \
	opkg_conf.h \
	opkg.h \
	opkg_message.h \
	nv_pair.h \
	nv_pair_list.h \
	pkg_depends.h \
	pkg_dest.h \
	pkg_dest_list.h \
	pkg.h \
	pkg_hash.h \
	pkg_src.h \
	pkg_src_list.h \
	pkg_vec.h \
	str_list.h \
	void_list.h \
	libopkg.h 

#\ replace/replace.h

#libopkg_la_LIBADD = libbb/libbb.la replace/libreplace.a
libopkg_la_LIBADD = libbb/libbb.la $(CURL_LIBS)

opkg_core_sources = args.c args.h opkg.c \
		    user.c user.h 
opkg_libcore_sources = args.c args.h libopkg.c libopkg.h opkg.h\
		    user.c user.h 
opkg_cmd_sources = opkg_cmd.c opkg_cmd.h \
		   opkg_configure.c opkg_configure.h \
		   opkg_download.c opkg_download.h \
		   opkg_install.c opkg_install.h \
		   opkg_upgrade.c opkg_upgrade.h \
		   opkg_remove.c opkg_remove.h
opkg_db_sources = opkg_conf.c opkg_conf.h \
		  opkg_utils.c opkg_utils.h pkg.c pkg.h hash_table.h \
		  pkg_depends.c pkg_depends.h pkg_extract.c pkg_extract.h \
		  hash_table.c pkg_hash.c pkg_hash.h pkg_parse.c pkg_parse.h \
		  pkg_vec.c pkg_vec.h
opkg_list_sources = conffile.c conffile.h conffile_list.c conffile_list.h \
		    nv_pair.c nv_pair.h nv_pair_list.c nv_pair_list.h \
		    pkg_dest.c pkg_dest.h pkg_dest_list.c pkg_dest_list.h \
		    pkg_src.c pkg_src.h pkg_src_list.c pkg_src_list.h \
		    str_list.c str_list.h void_list.c void_list.h
opkg_util_sources = file_util.c file_util.h opkg_message.h opkg_message.c md5.c md5.h \
		    sprintf_alloc.c sprintf_alloc.h str_util.c str_util.h \
		    xregex.c xregex.h xsystem.c xsystem.h

# opkg_SOURCES = $(opkg_core_sources) $(opkg_cmd_sources) $(opkg_db_sources) \
#	       $(opkg_util_sources) $(opkg_list_sources)

EXTRA_DIST = opkg.c update-alternatives $(intercept_DATA)

MAINTAINERCLEANFILES= \
	configure \
	Makefile.in \
	config.guess \
	config.sub \
	ltmain.sh \
	.Makefile.am.swp \
	aclocal.m4

libopkg_la_SOURCES = $(opkg_libcore_sources) $(opkg_cmd_sources) $(opkg_db_sources) \
	       $(opkg_util_sources) $(opkg_list_sources)

opkg_cl_SOURCES = opkg-frontend.c

libopkg_la_CFLAGS = -DOPKG_LIB $(ALL_CFLAGS)  $(CURL_CFLAGS)

opkg_CFLAGS = $(ALL_CFLAGS) $(CURL_CFLAGS)

opkg_cl_CFLAGS = -DOPKG_LIB $(ALL_CFLAGS)

noinst_PROGRAMS = opkg_hash_test opkg_extract_test

opkg_hash_test_LDADD = libbb/libbb.la
opkg_hash_test_SOURCES = opkg_hash_test.c $(opkg_db_sources) $(opkg_util_sources) $(opkg_list_sources)
opkg_hash_test_CFLAGS = $(ALL_CFLAGS)

opkg_extract_test_LDADD = libbb/libbb.la
opkg_extract_test_SOURCES = opkg_extract_test.c $(opkg_db_sources) $(opkg_util_sources) $(opkg_list_sources)
opkg_extract_test_CFLAGS = $(ALL_CFLAGS)

package: all-recursive
	STRIPPROG=$(STRIP) familiar/rules INSTALL=$$PWD/install-sh  binary-arch
