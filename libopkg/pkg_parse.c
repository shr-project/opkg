/* pkg_parse.c - the opkg package management system

   Steven M. Ayer
   
   Copyright (C) 2002 Compaq Computer Corporation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "includes.h"
#include <errno.h>
#include <ctype.h>
   
#include "pkg.h"
#include "opkg_utils.h"
#include "pkg_parse.h"
#include "libbb/libbb.h"

static int isGenericFieldType(char * type, const char * line)
{
    if(!strncmp(line, type, strlen(type)))
	return 1;
    return 0;
}

static char * parseGenericFieldType(char * type, const char * raw)
{
    const char * field_value = raw + (strlen(type) + 1);
    return trim_alloc(field_value);
}

static void parseStatus(pkg_t *pkg, const char * raw)
{
    char sw_str[64], sf_str[64], ss_str[64];

    sscanf(raw, "Status: %s %s %s", sw_str, sf_str, ss_str);
    pkg->state_want = pkg_state_want_from_str(sw_str);
    pkg->state_flag = pkg_state_flag_from_str(sf_str);
    pkg->state_status = pkg_state_status_from_str(ss_str);
}

static char ** parseDependsString(const char * raw, int * depends_count)
{
    char ** depends = NULL;
    int line_count = 0;
    char buff[2048], * dest;

    while(raw && *raw && !isspace(*raw)) {
	raw++;
    }

    if(line_is_blank(raw)){
	*depends_count = line_count;
	return NULL;
    }
    while(raw && *raw){
	depends = xrealloc(depends, sizeof(char *) * (line_count + 1));
	
	while(isspace(*raw)) raw++;

	dest = buff;
	while((*raw != ',') && *raw)
	    *dest++ = *raw++;

	*dest = '\0';
	depends[line_count] = trim_alloc(buff);
	if(depends[line_count] ==NULL)
	   return NULL;
        line_count++;
	if(*raw == ',')
	    raw++;
    }
    *depends_count = line_count;
    return depends;
}

static void parseConffiles(pkg_t * pkg, const char * raw)
{
    char file_name[1048], md5sum[1048];  /* please tell me there aren't any longer that 1k */

    if(!strncmp(raw, "Conffiles:", 10))
	raw += strlen("Conffiles:");

    while(*raw && (sscanf(raw, "%s%s", file_name, md5sum) == 2)){
	conffile_list_append(&pkg->conffiles, file_name, md5sum);
	/*	fprintf(stderr, "%s %s ", file_name, md5sum);*/
	while (*raw && isspace(*raw)) {
	    raw++;
	}
	raw += strlen(file_name);
	while (*raw && isspace(*raw)) {
	    raw++;
	}
	raw += strlen(md5sum);
    }
}    

int parseVersion(pkg_t *pkg, const char *raw)
{
  char *colon, *eepochcolon;
  char *hyphen;
  unsigned long epoch;

  if (!*raw) {
      fprintf(stderr, "%s: ERROR: version string is empty", __FUNCTION__);
      return EINVAL;
  }

  if (strncmp(raw, "Version:", 8) == 0) {
      raw += 8;
  }
  while (*raw && isspace(*raw)) {
      raw++;
  }
  
  colon= strchr(raw,':');
  if (colon) {
    epoch= strtoul(raw,&eepochcolon,10);
    if (colon != eepochcolon) {
	fprintf(stderr, "%s: ERROR: epoch in version is not number", __FUNCTION__);
	return EINVAL;
    }
    if (!*++colon) {
	fprintf(stderr, "%s: ERROR: nothing after colon in version number", __FUNCTION__);
	return EINVAL;
    }
    raw= colon;
    pkg->epoch= epoch;
  } else {
    pkg->epoch= 0;
  }

  pkg->revision = "";

  if (!pkg->version)
  {
    pkg->version= xcalloc(1, strlen(raw)+1);
    strcpy(pkg->version, raw);
  }

  hyphen= strrchr(pkg->version,'-');

  if (hyphen) {
    *hyphen++= 0;
      pkg->revision = hyphen;
  }

  return 0;
}

static int
pkg_parse_line(pkg_t *pkg, const char *line, uint mask)
{
	/* these flags are a bit hackish... */
	static int reading_conffiles = 0, reading_description = 0;

	switch (*line) {
	case 'A':
	    if((mask & PFM_ARCHITECTURE ) && isGenericFieldType("Architecture:", line))
		pkg->architecture = parseGenericFieldType("Architecture", line);
	    else if((mask & PFM_AUTO_INSTALLED) && isGenericFieldType("Auto-Installed:", line)) {
		char *auto_installed_value;
		auto_installed_value = parseGenericFieldType("Auto-Installed:", line);
		if (strcmp(auto_installed_value, "yes") == 0) {
		    pkg->auto_installed = 1;
		}
		free(auto_installed_value);
	    }
	    break;

	case 'C':
	    if((mask & PFM_CONFFILES) && isGenericFieldType("Conffiles", line)){
		parseConffiles(pkg, line);
		reading_conffiles = 1;
		reading_description = 0;
		goto dont_reset_flags;
	    }
	    else if((mask & PFM_CONFLICTS) && isGenericFieldType("Conflicts", line))
		pkg->conflicts_str = parseDependsString(line, &pkg->conflicts_count);
	    break;

	case 'D':
	    if((mask & PFM_DESCRIPTION) && isGenericFieldType("Description", line)) {
		pkg->description = parseGenericFieldType("Description", line);
		reading_conffiles = 0;
		reading_description = 1;
		goto dont_reset_flags;
	    }
	    else if((mask & PFM_DEPENDS) && isGenericFieldType("Depends", line))
		pkg->depends_str = parseDependsString(line, &pkg->depends_count);
	    break;

	case 'E':
	    if((mask & PFM_ESSENTIAL) && isGenericFieldType("Essential:", line)) {
		char *essential_value;
		essential_value = parseGenericFieldType("Essential", line);
		if (strcmp(essential_value, "yes") == 0) {
		    pkg->essential = 1;
		}
		free(essential_value);
	    }
	    break;

	case 'F':
	    if((mask & PFM_FILENAME) && isGenericFieldType("Filename:", line))
		pkg->filename = parseGenericFieldType("Filename", line);
	    break;

	case 'I':
	    if((mask && PFM_INSTALLED_SIZE) && isGenericFieldType("Installed-Size:", line))
		pkg->installed_size = parseGenericFieldType("Installed-Size", line);
	    else if((mask && PFM_INSTALLED_TIME) && isGenericFieldType("Installed-Time:", line)) {
		char *time_str = parseGenericFieldType("Installed-Time", line);
		pkg->installed_time = strtoul(time_str, NULL, 0);
		free (time_str);
	    }	    
	    break;

	case 'M':
	    if(mask && PFM_MD5SUM) {
		if (isGenericFieldType("MD5sum:", line))
			pkg->md5sum = parseGenericFieldType("MD5sum", line);
	    		/* The old opkg wrote out status files with the wrong
			 * case for MD5sum, let's parse it either way */
	    	else if(isGenericFieldType("MD5Sum:", line))
			pkg->md5sum = parseGenericFieldType("MD5Sum", line);
	    } else if((mask & PFM_MAINTAINER) && isGenericFieldType("Maintainer", line))
		pkg->maintainer = parseGenericFieldType("Maintainer", line);
	    break;

	case 'P':
	    if((mask & PFM_PACKAGE) && isGenericFieldType("Package:", line)) 
		pkg->name = parseGenericFieldType("Package", line);
	    else if((mask & PFM_PRIORITY) && isGenericFieldType("Priority:", line))
		pkg->priority = parseGenericFieldType("Priority", line);
	    else if((mask & PFM_PROVIDES) && isGenericFieldType("Provides", line)){
		pkg->provides_str = parseDependsString(line, &pkg->provides_count);
    	    } 
	    else if((mask & PFM_PRE_DEPENDS) && isGenericFieldType("Pre-Depends", line))
		pkg->pre_depends_str = parseDependsString(line, &pkg->pre_depends_count);
	    break;

	case 'R':
	    if((mask & PFM_RECOMMENDS) && isGenericFieldType("Recommends", line))
	        pkg->recommends_str = parseDependsString(line, &pkg->recommends_count);
	    else if((mask & PFM_REPLACES) && isGenericFieldType("Replaces", line))
		pkg->replaces_str = parseDependsString(line, &pkg->replaces_count);
	    
	    break;

	case 'S':
	    if((mask & PFM_SECTION) && isGenericFieldType("Section:", line))
		pkg->section = parseGenericFieldType("Section", line);
#ifdef HAVE_SHA256
	    else if((mask & PFM_SHA256SUM) && isGenericFieldType("SHA256sum:", line))
		pkg->sha256sum = parseGenericFieldType("SHA256sum", line);
#endif
	    else if((mask & PFM_SIZE) && isGenericFieldType("Size:", line))
		pkg->size = parseGenericFieldType("Size", line);
	    else if((mask & PFM_SOURCE) && isGenericFieldType("Source:", line))
		pkg->source = parseGenericFieldType("Source", line);
	    else if((mask & PFM_STATUS) && isGenericFieldType("Status", line))
		parseStatus(pkg, line);
	    else if((mask & PFM_SUGGESTS) && isGenericFieldType("Suggests", line))
		pkg->suggests_str = parseDependsString(line, &pkg->suggests_count);
	    break;

	case 'T':
	    if((mask & PFM_TAGS) && isGenericFieldType("Tags:", line))
		pkg->tags = parseGenericFieldType("Tags", line);
	    break;

	case 'V':
	    if((mask & PFM_VERSION) && isGenericFieldType("Version", line))
		parseVersion(pkg, line);
	    break;

	case ' ':
	    if((mask & PFM_DESCRIPTION) && reading_description) {
		/* we already know it's not blank, so the rest of description */      
		pkg->description = xrealloc(pkg->description,
					   strlen(pkg->description)
					   + 1 + strlen(line) + 1);
		strcat(pkg->description, "\n");
		strcat(pkg->description, (line));
		goto dont_reset_flags;
	    }
	    else if((mask && PFM_CONFFILES) && reading_conffiles) {
		parseConffiles(pkg, line);
		goto dont_reset_flags;
	    }
	    break;

	default:
	    /* For package lists, signifies end of package. */
	    if(line_is_blank(line)) {
		return 1;
	    }
	}

	reading_description = 0;
	reading_conffiles = 0;

dont_reset_flags:

	return 0;
}

int
pkg_parse_from_stream_nomalloc(pkg_t *pkg, FILE *fp, uint mask,
						char **buf0, size_t buf0len)
{
	int ret, lineno;
	char *buf, *nl;
	size_t buflen;

	lineno = 1;
	ret = 0;

	buflen = buf0len;
	buf = *buf0;
	buf[0] = '\0';

	while (1) {
		if (fgets(buf, buflen, fp) == NULL) {
			if (ferror(fp)) {
				fprintf(stderr, "%s: fgets: %s\n",
					__FUNCTION__, strerror(errno));
				ret = -1;
			} else if (strlen(*buf0) == buflen-1) {
				fprintf(stderr, "%s: missing new line character"
						" at end of file!\n",
					__FUNCTION__);
				pkg_parse_line(pkg, *buf0, mask);
			}
			break;
		}

		nl = strchr(buf, '\n');
		if (nl == NULL) {
			if (strlen(buf) < buflen-1) {
				/*
				 * Line could be exactly buflen-1 long and
				 * missing a newline, but we won't know until
				 * fgets fails to read more data.
				 */
				fprintf(stderr, "%s: missing new line character"
						" at end of file!\n",
					__FUNCTION__);
				pkg_parse_line(pkg, *buf0, mask);
				break;
			}
			if (buf0len >= EXCESSIVE_LINE_LEN) {
				fprintf(stderr, "%s: excessively long line at "
					"%d. Corrupt file?\n",
					__FUNCTION__, lineno);
				ret = -1;
				break;
			}

			/*
			 * Realloc and move buf past the data already read.
			 * |<--------------- buf0len ----------------->|
			 * |                     |<------- buflen ---->|
			 * |---------------------|---------------------|
			 * buf0                   buf
			 */
			buflen = buf0len;
			buf0len *= 2;
			*buf0 = xrealloc(*buf0, buf0len);
			buf = *buf0 + buflen -1;

			continue;
		}

		*nl = '\0';

		lineno++;

		if (pkg_parse_line(pkg, *buf0, mask))
			break;

		buf = *buf0;
		buflen = buf0len;
		buf[0] = '\0';
	};

	if (pkg->name == NULL) {
		/* probably just a blank line */
		ret = EINVAL;
	}

	return ret;
}

int
pkg_parse_from_stream(pkg_t *pkg, FILE *fp, uint mask)
{
	int ret;
	char *buf;
	const size_t len = 4096;

	buf = xmalloc(len);
	ret = pkg_parse_from_stream_nomalloc(pkg, fp, mask, &buf, len);
	free(buf);

	return ret;
}
