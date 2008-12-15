HOST_CPU=@host_cpu@
BUILD_CPU=@build_cpu@
ALL_CFLAGS=-g -O -Wall -DHOST_CPU_STR=\"@host_cpu@\" -DBUILD_CPU=@build_cpu@

noinst_LTLIBRARIES = libbb.la

libbb_la_SOURCES = gz_open.c \
	libbb.h \
	unzip.c \
	wfopen.c \
	unarchive.c \
	copy_file.c \
	copy_file_chunk.c \
	error_msg.c error_msg_and_die.c \
	perror_msg.c perror_msg_and_die.c \
	xreadlink.c \
	concat_path_file.c \
	ask_confirmation.c \
	verror_msg.c vperror_msg.c \
	xfuncs.c messages.c \
	last_char_is.c \
	make_directory.c \
	safe_strncpy.c \
	parse_mode.c \
	time_string.c \
	all_read.c \
	mode_string.c

libbb_la_CFLAGS = $(ALL_CFLAGS)
#libbb_la_LDFLAGS = -static
