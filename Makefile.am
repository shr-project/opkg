#SUBDIRS = etc replace familiar libbb
SUBDIRS = etc familiar libbb

HOST_CPU=@host_cpu@
BUILD_CPU=@build_cpu@
IPKGLIBDIR=@ipkglibdir@ 
ALL_CFLAGS=-g -O -Wall -DHOST_CPU_STR=\"@host_cpu@\" -DBUILD_CPU=@build_cpu@ -DLIBDIR=\"@libdir@\" -DIPKGLIBDIR=\"@ipkglibdir@\" -DDATADIR=\"@datadir@\"

bin_PROGRAMS = ipkg-cl

lib_LTLIBRARIES = libipkg.la

bin_SCRIPTS = update-alternatives

interceptdir = $(datadir)/ipkg/intercept
intercept_DATA = intercept/ldconfig intercept/depmod intercept/update-modules

install-data-hook:
	chmod +x $(DESTDIR)$(datadir)/ipkg/intercept/*

# ipkg_LDADD = libbb/libbb.a replace/libreplace.a

#ipkg_cl_LDADD = libipkg.la libbb/libbb.la replace/libreplace.a 
ipkg_cl_LDADD = libipkg.la libbb/libbb.la 

libipkg_includedir=$(includedir)/libipkg

libipkg_include_HEADERS= \
	args.h \
	conffile.h \
	conffile_list.h \
	config.h \
	hash_table.h \
	includes.h \
	ipkg_conf.h \
	ipkg.h \
	ipkg_message.h \
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
	libipkg.h 

#\ replace/replace.h

#libipkg_la_LIBADD = libbb/libbb.la replace/libreplace.a
libipkg_la_LIBADD = libbb/libbb.la 

ipkg_core_sources = args.c args.h ipkg.c \
		    user.c user.h 
ipkg_libcore_sources = args.c args.h libipkg.c libipkg.h ipkg.h\
		    user.c user.h 
ipkg_cmd_sources = ipkg_cmd.c ipkg_cmd.h \
		   ipkg_configure.c ipkg_configure.h \
		   ipkg_download.c ipkg_download.h \
		   ipkg_install.c ipkg_install.h \
		   ipkg_upgrade.c ipkg_upgrade.h \
		   ipkg_remove.c ipkg_remove.h
ipkg_db_sources = ipkg_conf.c ipkg_conf.h \
		  ipkg_utils.c ipkg_utils.h pkg.c pkg.h hash_table.h \
		  pkg_depends.c pkg_depends.h pkg_extract.c pkg_extract.h \
		  hash_table.c pkg_hash.c pkg_hash.h pkg_parse.c pkg_parse.h \
		  pkg_vec.c pkg_vec.h
ipkg_list_sources = conffile.c conffile.h conffile_list.c conffile_list.h \
		    nv_pair.c nv_pair.h nv_pair_list.c nv_pair_list.h \
		    pkg_dest.c pkg_dest.h pkg_dest_list.c pkg_dest_list.h \
		    pkg_src.c pkg_src.h pkg_src_list.c pkg_src_list.h \
		    str_list.c str_list.h void_list.c void_list.h
ipkg_util_sources = file_util.c file_util.h ipkg_message.h ipkg_message.c md5.c md5.h \
		    sprintf_alloc.c sprintf_alloc.h str_util.c str_util.h \
		    xregex.c xregex.h xsystem.c xsystem.h

# ipkg_SOURCES = $(ipkg_core_sources) $(ipkg_cmd_sources) $(ipkg_db_sources) \
#	       $(ipkg_util_sources) $(ipkg_list_sources)

EXTRA_DIST = ipkg.c update-alternatives $(intercept_DATA)

MAINTAINERCLEANFILES= \
	configure \
	Makefile.in \
	config.guess \
	config.sub \
	ltmain.sh \
	.Makefile.am.swp \
	aclocal.m4

libipkg_la_SOURCES = $(ipkg_libcore_sources) $(ipkg_cmd_sources) $(ipkg_db_sources) \
	       $(ipkg_util_sources) $(ipkg_list_sources)

ipkg_cl_SOURCES = ipkg-frontend.c

libipkg_la_CFLAGS = -DIPKG_LIB $(ALL_CFLAGS)

ipkg_CFLAGS = $(ALL_CFLAGS)

ipkg_cl_CFLAGS = -DIPKG_LIB $(ALL_CFLAGS)

noinst_PROGRAMS = ipkg_hash_test ipkg_extract_test

ipkg_hash_test_LDADD = libbb/libbb.la
ipkg_hash_test_SOURCES = ipkg_hash_test.c $(ipkg_db_sources) $(ipkg_util_sources) $(ipkg_list_sources)
ipkg_hash_test_CFLAGS = $(ALL_CFLAGS)

ipkg_extract_test_LDADD = libbb/libbb.la
ipkg_extract_test_SOURCES = ipkg_extract_test.c $(ipkg_db_sources) $(ipkg_util_sources) $(ipkg_list_sources)
ipkg_extract_test_CFLAGS = $(ALL_CFLAGS)

package: all-recursive
	STRIPPROG=$(STRIP) familiar/rules INSTALL=$$PWD/install-sh  binary-arch
