
ALL_CFLAGS=-g -O -Wall -Werror -DHOST_CPU_STR=\"@host_cpu@\" -DBUILD_CPU=@build_cpu@ -DLIBDIR=\"@libdir@\" -DOPKGLIBDIR=\"@opkglibdir@\" -DDATADIR=\"@datadir@\" -I$(top_srcdir)

libopkg_includedir=$(includedir)/libopkg

libopkg_include_HEADERS= opkg.h

libopkg_la_LIBADD = $(top_builddir)/libbb/libbb.la $(CURL_LIBS) $(GPGME_LIBS)

# make sure we only export symbols that are for public use
libopkg_la_LDFLAGS = -export-symbols-regex "^opkg_.*"

opkg_core_sources = args.c args.h \
		    user.c user.h 
opkg_libcore_sources = args.c args.h libopkg.c libopkg.h\
		    user.c user.h opkg_state.c opkg_state.h \
		    opkg.c opkg.h
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

libopkg_la_SOURCES = \
	$(opkg_libcore_sources) \
	$(opkg_cmd_sources) $(opkg_db_sources) \
	$(opkg_util_sources) $(opkg_list_sources)

libopkg_la_CFLAGS = $(ALL_CFLAGS) $(BIGENDIAN_CFLAGS) $(CURL_CFLAGS) $(GPGME_CFLAGS)
lib_LTLIBRARIES = libopkg.la


