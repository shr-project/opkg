# Process this file with autoconf to produce a configure script
AC_INIT(libopkg/libopkg.c)
AM_INIT_AUTOMAKE([opkg], [0.1.4])
AM_CONFIG_HEADER(libopkg/config.h)

AC_CANONICAL_HOST
AC_GNU_SOURCE 


for top_builddir in . .. ../.. $ac_auxdir $ac_auxdir/..; do
  test -f $top_builddir/configure && break
done


# Checks for programs
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_INSTALL_STRIP
AC_PROG_LIBTOOL

# Checks for libraries

# check for libcurl
PKG_CHECK_MODULES(CURL, libcurl)



dnl **********
dnl GPGME
dnl **********

AC_ARG_ENABLE(gpg,
    AC_HELP_STRING([--enable-gpg], [Enable signature checking with gpgme
      [[default=yes]] ]),
    [want_gpgme="$enableval"], [want_gpgme="yes"])

if test "x$want_gpgme" = "xyes"; then
  ok="no"
  min_gpgme_version=1.0.0
  AC_PATH_PROG(GPGME_CONFIG, gpgme-config, "failed")
  if test $GPGME_CONFIG != "failed" ; then
	AC_MSG_CHECKING(for GPGME - version >= $min_gpgme_version)
	req_major=`echo $min_gpgme_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1/'`
	req_minor=`echo $min_gpgme_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\2/'`
	req_micro=`echo $min_gpgme_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\3/'`
	gpgme_config_version=`$GPGME_CONFIG --version`
	major=`echo $gpgme_config_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
	minor=`echo $gpgme_config_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
	micro=`echo $gpgme_config_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/'`

	  if test "$major" -eq "$req_major"; then
	     if test "$minor" -ge "$req_minor"; then
	       if test "$micro" -ge "$req_micro"; then
		 ok="yes"
	       fi
	     fi
	  fi
  fi

  if test $ok = "yes"; then
    GPGME_CFLAGS=`$GPGME_CONFIG --cflags`
    GPGME_LIBS=`$GPGME_CONFIG --libs`
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GPGME, 1, [Define if you want GPG support])
  else
    AC_MSG_ERROR(GPGME $min_gpgme_version or later needed)
  fi
fi

AC_SUBST(GPGME_CFLAGS)
AC_SUBST(GPGME_LIBS)


# Checks for header files
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([errno.h fcntl.h memory.h regex.h stddef.h stdlib.h string.h strings.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks endianness
AC_C_BIGENDIAN(BIGENDIAN_CFLAGS="-DWORDS_BIGENDIAN=1",)
AC_SUBST(BIGENDIAN_CFLAGS)

# Don't do annoying tests that don't work when cross-compiling, just trust them.
# The AC_FUNC_MEMCMP test doesn't work during a cross-compile, disable.
# AC_FUNC_MEMCMP
# The AC_FUNC_STAT test doesn't work during a cross-compile, disable.
# AC_FUNC_STAT

# Checks for library functions
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_TYPE_SIGNAL
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memmove memset mkdir regcomp strchr strcspn strdup strerror strndup strrchr strstr strtol strtoul sysinfo utime])

opkglibdir=
AC_ARG_WITH(opkglibdir,
[  --with-opkglibdir=DIR   specifies directory to put status and info files.
                          "/opkg" is always added so if you want your files 
                          to be in /usr/lib/opkg you should indicate 
                          --with-opkglibdir=/usr/lib ],
[case "${withval}" in
yes)    AC_MSG_ERROR(bad value ${withval} given for opkg libs directories ) ;;
no)     ;;
*)      opkglibdir=$with_opkglibdir ;;
esac])

# Default local prefix if it is empty
if test x$opkglibdir = x; then
        opkglibdir=/usr/lib
fi

AC_SUBST(opkglibdir)

AC_OUTPUT(
    Makefile
    libopkg/Makefile
    tests/Makefile
    src/Makefile
    libbb/Makefile
    utils/Makefile
    libopkg.pc
    )
