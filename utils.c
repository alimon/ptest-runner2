/**
 * Copyright (c) 2016 Intel Corporation
 * Copyright (C) 2019 Wind River Systems, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * AUTHORS
 * 	Aníbal Limón <anibal.limon@intel.com>
 */

#define _GNU_SOURCE

#include <stdio.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "ptest_list.h"
#include "utils.h"

#define GET_STIME_BUF_SIZE 1024
#define WAIT_CHILD_BUF_MAX_SIZE 1024

#define UNUSED(x) (void)(x)

enum {
	PIPE_READ = 0,
	PIPE_WRITE = 1,
};

static inline char *
get_stime(char *stime, size_t size, time_t t)
{
	struct tm *lt;

	lt = localtime(&t);
	strftime(stime, size, "%Y-%m-%dT%H:%M", lt);

	return stime;
}

void
check_allocation1(void *p, size_t size, char *file, int line, int exit_on_null)
{
	if (p == NULL) {
		fprintf(stderr, "Failed to allocate memory %ld bytes at %s,"
				" line %d.\n", size, file, line);
		if (exit_on_null)
			exit(1);
	}
}

static void
set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) {
		fprintf(stderr, "Unable to get flags for FD %d: %s\n", fd, strerror(errno));
		return;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		fprintf(stderr, "Unable to set flags for FD %d: %s\n", fd, strerror(errno));
	}
}

static void
do_close(int *fd)
{
	if (*fd >= 0) {
		close(*fd);
		*fd = -1;
	}
}


struct ptest_list *
get_available_ptests(const char *dir)
{
	struct ptest_list *head;
	struct stat st_buf;

	int n, i;
	struct dirent **namelist;
	int fail;
	int saved_errno = -1; /* Initalize to invalid errno. */
	char realdir[PATH_MAX];

	realpath(dir, realdir);

	do
	{
		head = ptest_list_alloc();
		CHECK_ALLOCATION(head, sizeof(struct ptest_list *), 0);
		if (head == NULL)
			break;

		if (stat(realdir, &st_buf) == -1) {
			PTEST_LIST_FREE_CLEAN(head);
			break;
		}

		if (!S_ISDIR(st_buf.st_mode)) {
			PTEST_LIST_FREE_CLEAN(head);
			errno = EINVAL;
			break;
		}

		n = scandir(realdir, &namelist, NULL, alphasort);
		if (n == -1) {
			PTEST_LIST_FREE_CLEAN(head);
			break;
		}


		fail = 0;
		for (i = 0; i < n; i++) {
			char *run_ptest;

			char *d_name = strdup(namelist[i]->d_name);
			CHECK_ALLOCATION(d_name, sizeof(namelist[i]->d_name), 0);
			if (d_name == NULL) {
				fail = 1;
				saved_errno = errno;
				break;
			}

			if (strcmp(d_name, ".") == 0 ||
			    strcmp(d_name, "..") == 0) {
				free(d_name);
				continue;
			}

			if (asprintf(&run_ptest, "%s/%s/ptest/run-ptest",
			    realdir, d_name) == -1)  {
				fail = 1;
				saved_errno = errno;
				free(d_name);
				break;
			}

			if (stat(run_ptest, &st_buf) == -1) {
				free(run_ptest);
				free(d_name);
				continue;
			}

			if (!S_ISREG(st_buf.st_mode)) {
				free(run_ptest);
				free(d_name);
				continue;
			}

			if (ptest_list_search_by_file(head, run_ptest, st_buf)) {
				free(run_ptest);
				free(d_name);
				continue;
			}

			struct ptest_list *p = ptest_list_add(head,
				d_name, run_ptest);
			CHECK_ALLOCATION(p, sizeof(struct ptest_list *), 0);
			if (p == NULL) {
				fail = 1;
				saved_errno = errno;
				free(run_ptest);
				free(d_name);
				break;
			}
		}

		for (i = 0 ; i < n; i++)
			free(namelist[i]);
		free(namelist);

		if (fail) {
			PTEST_LIST_FREE_ALL_CLEAN(head);
			errno = saved_errno;
			break;
		}
	} while (0);

	return head;
}

int
print_ptests(struct ptest_list *head, FILE *fp)
{
	if (head == NULL || ptest_list_length(head) <= 0) {
		fprintf(fp, PRINT_PTESTS_NOT_FOUND);
		return 1;
	} else {
		struct ptest_list *n;
		fprintf(fp, PRINT_PTESTS_AVAILABLE);
		PTEST_LIST_ITERATE_START(head, n)
			fprintf(fp, "%s\t%s\n", n->ptest, n->run_ptest);
		PTEST_LIST_ITERATE_END
		return 0;
	}
}

struct ptest_list *
filter_ptests(struct ptest_list *head, char **ptests, int ptest_num)
{
	struct ptest_list *head_new = NULL, *n;
	int fail = 0, i, saved_errno = 0;

	do {
		if (head == NULL || ptests == NULL || ptest_num <= 0) {
			errno = EINVAL;
			break;
		}

		head_new = ptest_list_alloc();
		if (head_new == NULL)
			break;

		for (i = 0; i < ptest_num; i++) {
			char *ptest;
			char *run_ptest;

			n = ptest_list_search(head, ptests[i]);
			if (n == NULL) {
				saved_errno = errno;
				fail = 1;
				break;
			}

			ptest = strdup(n->ptest);
			run_ptest = strdup(n->run_ptest);
			if (ptest == NULL || run_ptest == NULL) {
				saved_errno = errno;
				fail = 1;
				break;
			}

			if (ptest_list_add(head_new, ptest, run_ptest) == NULL) {
				saved_errno = errno;
				fail = 1;
				break;
			}
		}

		if (fail) {
			PTEST_LIST_FREE_ALL_CLEAN(head_new);
			errno = saved_errno;
		}
	} while (0);

	return head_new;
}

/* Close all fds from 3 up to 'ulimit -n'
 * i.e. do not close STDIN, STDOUT, STDERR.
 * Typically called in in a child process after forking
 * but before exec as a good policy especially for security.
 */
static void
close_fds(void)
{
	struct rlimit curr_lim;
	getrlimit(RLIMIT_NOFILE, &curr_lim);

	int fd;
	for (fd=3; fd < (int)curr_lim.rlim_cur; fd++) {
		(void) close(fd);
   	}
}

static void
collect_system_state(FILE* fout)
{
	char *cmd = "ptest-runner-collect-system-data";

	char buf[1024];
	FILE *fp;

	if ((fp = popen(cmd, "r")) == NULL) {
		fprintf(fout, "Error opening pipe!\n");
	}

	while (fgets(buf, 1024, fp) != NULL) {
		fprintf(fout, "%s", buf);
	}

	if(pclose(fp))  {
		fprintf(fout, "Command not found or exited with error status\n");
	}
}

static inline void
run_child(char *run_ptest, int fd_stdout, int fd_stderr)
{
	char *const argv[2] = {run_ptest, NULL};
	chdir(dirname(strdup(run_ptest)));

	dup2(fd_stdout, STDOUT_FILENO);
	// XXX: Redirect stderr to stdout to avoid buffer ordering problems.
	dup2(fd_stdout, STDERR_FILENO);

	/* since it isn't use by the child, close(fd_stderr) ? */
	close(fd_stderr); /* try using to see if this fixes bash run-read. rwm todo */
	close_fds();

	execv(run_ptest, argv);

	/* exit(1); not needed? */
}

/* Returns an integer file descriptor.
 * If it returns < 0, an error has occurred.
 * Otherwise, it has returned the slave pty file descriptor.
 * fp should be writable, likely stdout/err.
 */
static int
setup_slave_pty(FILE *fp) {
	int pty_master = -1;
	int pty_slave = -1;
	char pty_name[256];
	struct group *gptr;
	gid_t gid;
	int slave = -1;

	if (openpty(&pty_master, &pty_slave, pty_name, NULL, NULL) < 0) {
		fprintf(fp, "ERROR: openpty() failed with: %s.\n", strerror(errno));
		return -1;
	}

	if ((gptr = getgrnam(pty_name)) != 0) {
		gid = gptr->gr_gid;
	} else {
		/* If the tty group does not exist, don't change the
		 * group on the slave pty, only the owner
		 */
		gid = (gid_t)-1;
	}

	/* chown/chmod the corresponding pty, if possible.
	 * This will only work if the process has root permissions.
	 */
	if (chown(pty_name, getuid(), gid) != 0) {
		fprintf(fp, "ERROR; chown() failed with: %s.\n", strerror(errno));
	}

	/* Makes the slave read/writeable for the user. */
	if (chmod(pty_name, S_IRUSR|S_IWUSR) != 0) {
		fprintf(fp, "ERROR: chmod() failed with: %s.\n", strerror(errno));
	}

	if ((slave = open(pty_name, O_RDWR)) == -1) {
		fprintf(fp, "ERROR: open() failed with: %s.\n", strerror(errno));
	}
	return (slave);
}


int
run_ptests(struct ptest_list *head, const struct ptest_options opts,
		const char *progname, FILE *fp, FILE *fp_stderr)
{
	int rc = 0;
	FILE *xh = NULL;

	struct ptest_list *p;


	if (opts.xml_filename) {
		xh = xml_create(ptest_list_length(head), opts.xml_filename);
		if (!xh)
			exit(EXIT_FAILURE);
	}

	do
	{
		if (isatty(0) && ioctl(0, TIOCNOTTY) == -1) {
			fprintf(fp, "ERROR: Unable to detach from controlling tty, %s\n", strerror(errno));
		}


		fprintf(fp, "START: %s\n", progname);
		PTEST_LIST_ITERATE_START(head, p)
			int pipefd_stdout[2] = {-1, -1};
			int pipefd_stderr[2] = {-1, -1};
			int pgid = -1;

			if ((rc = pipe2(pipefd_stdout, 0)) == -1)
				break;

			if ((rc = pipe2(pipefd_stderr, 0)) == -1) {
				close(pipefd_stdout[PIPE_READ]);
				close(pipefd_stdout[PIPE_WRITE]);
				break;
			}

			char *ptest_dir = strdup(p->run_ptest);
			if (ptest_dir == NULL) {
				rc = -1;
				break;
			}
			dirname(ptest_dir);

			if ((pgid = getpgid(0)) == -1) {
				fprintf(fp, "ERROR: getpgid() failed, %s\n", strerror(errno));
			}

			pid_t child = fork();
			if (child == -1) {
				fprintf(fp, "ERROR: Fork %s\n", strerror(errno));
				rc = -1;
				break;
			} else if (child == 0) {
				int slave;

				close(0);
				/* Close read ends of the pipe */
				do_close(&pipefd_stdout[PIPE_READ]);
				do_close(&pipefd_stderr[PIPE_READ]);

				if ((slave = setup_slave_pty(fp)) < 0) {
					fprintf(fp, "ERROR: could not setup pty (%d).", slave);
				}
				if (setpgid(0,pgid) == -1) {
					fprintf(fp, "ERROR: setpgid() failed, %s\n", strerror(errno));
				}

				if (setsid() ==  -1) {
					fprintf(fp, "ERROR: setsid() failed, %s\n", strerror(errno));
				}

				if (ioctl(0, TIOCSCTTY, NULL) == -1) {
					fprintf(fp, "ERROR: Unable to attach to controlling tty, %s\n", strerror(errno));
				}

				run_child(p->run_ptest, pipefd_stdout[PIPE_WRITE], pipefd_stderr[PIPE_WRITE]);

			} else {
				bool timedout = false;
				char stime[GET_STIME_BUF_SIZE];

				/* Close write ends of the pipe, otherwise this process will never get EOF when the child dies */
				do_close(&pipefd_stdout[PIPE_WRITE]);
				do_close(&pipefd_stderr[PIPE_WRITE]);

				if (setpgid(child, pgid) == -1) {
					fprintf(fp, "ERROR: setpgid() failed, %s\n", strerror(errno));
				}

				time_t start_time= time(NULL);
				fprintf(fp, "%s\n", get_stime(stime, GET_STIME_BUF_SIZE, start_time));
				fprintf(fp, "BEGIN: %s\n", ptest_dir);

				struct pollfd pfds[2];
				FILE* dest_fps[2];
				set_nonblocking(pipefd_stdout[PIPE_READ]);
				pfds[0].fd = pipefd_stdout[PIPE_READ];
				pfds[0].events = POLLIN;
				dest_fps[0] = fp;

				set_nonblocking(pipefd_stderr[PIPE_READ]);
				pfds[1].fd = pipefd_stderr[PIPE_READ];
				pfds[1].events = POLLIN;
				dest_fps[1] = fp_stderr;

				while (true) {
					/*
					 * Check all the poll file descriptors.
					 * Only when all of them are done
					 * (negative) will we exit the poll()
					 * loop
					 */
					bool done = true;
					for (int i = 0; i < 2; i++) {
						if (pfds[i].fd >= 0) {
							done = false;
							break;
						}
					}

					if (done) {
						break;
					}

					int ret = poll(pfds, 2, opts.timeout*1000);

					if (ret == 0 && !timedout) {
						/* kill the child if we haven't
						 * already. Note that we
						 * continue to read data from
						 * the pipes until EOF to make
						 * sure we get all the output
						 */
						kill(-child, SIGKILL);
						timedout = true;
					}

					for (int i = 0; i < 2; i++) {
						if (pfds[i].revents & (POLLIN | POLLHUP)) {
							char buf[WAIT_CHILD_BUF_MAX_SIZE];
							ssize_t n = read(pfds[i].fd, buf, sizeof(buf));

							if (n == 0) {
								/* Closed */
								pfds[i].fd = -1;
								continue;
							}

							if (n < 0) {
								if (errno != EAGAIN && errno != EWOULDBLOCK) {
									pfds[i].fd = -1;
									fprintf(stderr, "Error reading from stream %d: %s\n", i, strerror(errno));
								}
								continue;
							} else {
								fwrite(buf, (size_t)n, 1, dest_fps[i]);
							}
						}
					}
				}

				if (timedout) {
					collect_system_state(fp);
				} else {
					/*
					 * This kill is just in case the child did
					 * something really silly like close its
					 * stdout and stderr but then go into an
					 * infinite loop and never exit. Normally, it
					 * will just fail because the child is already
					 * dead
					 */
					kill(-child, SIGKILL);
				}
				int status;
				waitpid(child, &status, 0);

				time_t end_time = time(NULL);
				time_t duration = end_time - start_time;

				int exit_code = -1;
				if (WIFEXITED(status)) {
					exit_code = WEXITSTATUS(status);
					if (exit_code) {
						fprintf(fp, "\nERROR: Exit status is %d\n", exit_code);
						rc += 1;
					}
				} else if (WIFSIGNALED(status)) {
					int signal = WTERMSIG(status);
					fprintf(fp, "\nERROR: Exited from signal %s (%d)\n", strsignal(signal), signal);
					rc += 1;
				} else {
					fprintf(fp, "\nERROR: Exited for unknown reason (%d)\n", status);
					rc += 1;
				}
				fprintf(fp, "DURATION: %d\n", (int) duration);
				if (timedout) {
					fprintf(fp, "TIMEOUT: %s\n", ptest_dir);
					rc += 1;
				}

				if (opts.xml_filename)
					xml_add_case(xh, exit_code, ptest_dir, timedout, (int) duration);

				fprintf(fp, "END: %s\n", ptest_dir);
				fprintf(fp, "%s\n", get_stime(stime, GET_STIME_BUF_SIZE, end_time));
			}
			free(ptest_dir);
			do_close(&pipefd_stdout[PIPE_READ]);
			do_close(&pipefd_stdout[PIPE_WRITE]);
			do_close(&pipefd_stderr[PIPE_READ]);
			do_close(&pipefd_stderr[PIPE_WRITE]);

		PTEST_LIST_ITERATE_END
		fprintf(fp, "STOP: %s\n", progname);
	} while (0);

	if (rc == -1)
		fprintf(fp_stderr, "run_ptests fails: %s", strerror(errno));

	if (opts.xml_filename)
		xml_finish(xh);

	return rc;
}

FILE *
xml_create(int test_count, char *xml_filename)
{
	FILE *xh;

	if ((xh = fopen(xml_filename, "w"))) {
		fprintf(xh, "<?xml version='1.0' encoding='UTF-8'?>\n");
		fprintf(xh, "<testsuite name='ptest' tests='%d'>\n", test_count);
	} else {
		fprintf(stderr, "XML File '%s' could not be created. %s.\n",
				xml_filename, strerror(errno));
		return NULL;
	}

	return xh;
}

void
xml_add_case(FILE *xh, int status, const char *ptest_dir, int timeouted, int duration)
{
	fprintf(xh, "\t<testcase classname='%s' name='run-ptest'>\n", ptest_dir);
	fprintf(xh, "\t\t<duration>%d</duration>\n", duration);

	if (status != 0) {
		fprintf(xh, "\t\t<failure type='exit_code'");
		fprintf(xh, " message='run-ptest exited with code: %d'>", status);
		fprintf(xh, "</failure>\n");
	}
	if (timeouted)
		fprintf(xh, "\t\t<failure type='timeout'/>\n");

	fprintf(xh, "\t</testcase>\n");
}

void
xml_finish(FILE *xh)
{
	fprintf(xh, "</testsuite>\n");
	fclose(xh);
}
