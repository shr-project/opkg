#ifndef INCLUDES_H
#define INCLUDES_H

#include "config.h"
#include <stdio.h>

#if STDC_HEADERS
# include <stdlib.h>
# include <stdarg.h>
# include <stddef.h>
# include <ctype.h>
# include <errno.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#if HAVE_REGEX_H
# include <regex.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if HAVE_STRINGS_H
# include <strings.h>
#endif

#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#if HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
#endif

// #include "replace/replace.h"

#endif
