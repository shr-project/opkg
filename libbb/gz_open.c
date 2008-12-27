/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) many different people.  If you wrote this, please
 * acknowledge your work.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libbb.h"

extern FILE *gz_open(FILE *compressed_file, int *pid)
{
	int unzip_pipe[2];

	if (pipe(unzip_pipe)!=0) {
		error_msg("pipe error");
		return(NULL);
	}
	if ((*pid = fork()) == -1) {
		error_msg("fork failed");
		return(NULL);
	}
	if (*pid==0) {
		/* child process */
		close(unzip_pipe[0]);
		unzip(compressed_file, fdopen(unzip_pipe[1], "w"));
		fflush(NULL);
		fclose(compressed_file);
		close(unzip_pipe[1]);
		exit(EXIT_SUCCESS);
	}
	close(unzip_pipe[1]);
	if (unzip_pipe[0] == -1) {
		error_msg("gzip stream init failed");
	}
	return(fdopen(unzip_pipe[0], "r"));
}

/* gz_open implementation using gunzip and a vfork/exec -- dodges OOM killer */
extern FILE *gzvopen(FILE *compressed_file, int *pid)
{
	int unzip_pipe[2];
	off_t floc;
	int cfile;

	/* create a new file descriptor for the input stream
	 * (it *must* be associated with a file)
	 * and seek to the same position in that fd as the stream.
	 */
	cfile = dup(fileno(compressed_file));
	floc = ftello(compressed_file);
	lseek(cfile, floc, SEEK_SET);

	/* create the pipe */
	if (pipe(unzip_pipe)!=0) {
		error_msg("gzvopen(): pipe error");
		return(NULL);
	}

	*pid = vfork();

	if (*pid < 0) {
		error_msg("gzvopen(): fork failed");
		return(NULL);
	}

	if (*pid==0) {
		/* child process - reads STDIN, writes to pipe */

		/* close unused read end of pipe */
		close(unzip_pipe[0]);

		/* connect child's stdout to the pipe write end */
		dup2(unzip_pipe[1], 1);

		/* connect child's stdin to the fd passed in to us */
		dup2(cfile, 0);

		/* execute the gunzip utility */
		execlp("gunzip","gunzip",NULL);

		/* if we get here, we had a failure - since we are
		 * using vfork(), we cannot call exit(), must call _exit().
		 */
		_exit(-1);
	}

	/* Parent process is executing here */

	/* we have no more need of the duplicate fd */
	close(cfile);

	/* close the write end of the pipe */
	close(unzip_pipe[1]);

	/* return the read end of the pipe as a FILE */
	return(fdopen(unzip_pipe[0], "r"));
}

extern void gzvclose(int gunzip_pid)
{
        if (kill(gunzip_pid, SIGTERM) == -1) {
		perror("gzvclose()");
                fprintf(stderr,"%s: unable to kill gunzip pid.\n",
			__FUNCTION__);
        }

        if (waitpid(gunzip_pid, NULL, 0) == -1) {
                fprintf(stderr,"%s: unable to wait on gunzip pid.\n",
			__FUNCTION__);
        }
}
