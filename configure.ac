# Process this file with autoconf to produce a configure script
AC_INIT(ipkg.c)
AM_INIT_AUTOMAKE([ipkg], [0.99.163])
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST


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

# Checks for libraries (hah! ipkg is pretty much self-contained)

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

ipkglibdir=
AC_ARG_WITH(ipkglibdir,
[  --with-ipkglibdir=DIR   specifies directory to put status and info files.
                          "/ipkg" is always added so if you want your files 
                          to be in /usr/lib/ipkg you should indicate 
                          --with-ipkglibdir=/usr/lib ],
[case "${withval}" in
yes)    AC_MSG_ERROR(bad value ${withval} given for ipkg libs directories ) ;;
no)     ;;
*)      ipkglibdir=$with_ipkglibdir ;;
esac])

# Default local prefix if it is empty
if test x$ipkglibdir = x; then
        ipkglibdir=/usr/lib
fi

AC_SUBST(ipkglibdir)

AC_OUTPUT(Makefile etc/Makefile familiar/Makefile familiar/control familiar/control-unstripped familiar/libipkg-control  familiar/libipkg-dev-control libbb/Makefile libipkg.pc ipkg.h)
