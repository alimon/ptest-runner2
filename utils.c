/**
 * Copyright (c) 2016 Intel Corporation
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
 * AUTHORS
 * 	Aníbal Limón <anibal.limon@intel.com>
 */

#define _GNU_SOURCE 
#include <stdio.h>

#include <libgen.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <errno.h>

#include "ptest_list.h"
#include "utils.h"

#define GET_STIME_BUF_SIZE 1024
#define WAIT_CHILD_POLL_TIMEOUT_MS 200
#define WAIT_CHILD_BUF_MAX_SIZE 1024

static inline char *
get_stime(char *stime, size_t size)
{
	time_t t;
	struct tm *lt;

	t = time(NULL);
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


struct ptest_list *
get_available_ptests(const char *dir)
{
	struct ptest_list *head;
	struct stat st_buf;

	int n, i;
	struct dirent **namelist;
	int fail;
	int saved_errno;

	do
	{
		head = ptest_list_alloc();
		CHECK_ALLOCATION(head, sizeof(struct ptest_list *), 0);
		if (head == NULL)
			break;

		if (stat(dir, &st_buf) == -1) {
			PTEST_LIST_FREE_CLEAN(head);
			break;
		}

		if (!S_ISDIR(st_buf.st_mode)) {
			PTEST_LIST_FREE_CLEAN(head);
			errno = EINVAL;
			break;
		}

		n = scandir(dir, &namelist, NULL, alphasort);
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
			    dir, d_name) == -1)  {
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
		PTEST_LIST_ITERATE_START(head, n);
			fprintf(fp, "%s\t%s\n", n->ptest, n->run_ptest);
		PTEST_LIST_ITERATE_END;
		return 0;
	}
}

struct ptest_list *
filter_ptests(struct ptest_list *head, char **ptests, int ptest_num)
{
	struct ptest_list *head_new = NULL, *n;
	int fail = 0, i, saved_errno;

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

static inline void
run_child(char *run_ptest, int fd_stdout, int fd_stderr)
{
	char **argv = malloc(sizeof(char) * 2);
	chdir(dirname(strdup(run_ptest)));

	argv[0] = run_ptest;
	argv[1] = NULL;

	dup2(fd_stdout, STDOUT_FILENO);
	dup2(fd_stderr, STDERR_FILENO);
	execv(run_ptest, argv);

	exit(1);
}

static inline int
wait_child(const char *ptest_dir, const char *run_ptest, pid_t pid,
		int timeout, int *fds, FILE **fps)
{
	struct pollfd pfds[2];
	time_t sentinel;
	int r;

	int timeouted = 0;
	int status;
	int waitflags;

	pfds[0].fd = fds[0];
	pfds[0].events = POLLIN;
	pfds[1].fd = fds[1];
	pfds[1].events = POLLIN;

	sentinel = time(NULL);
	while (1) {
		waitflags = WNOHANG;

		r = poll(pfds, 2, WAIT_CHILD_POLL_TIMEOUT_MS);
		if (r > 0) {
			char buf[WAIT_CHILD_BUF_MAX_SIZE];
			ssize_t n;

			if (pfds[0].revents != 0) {
				while ((n = read(fds[0], buf, WAIT_CHILD_BUF_MAX_SIZE)) > 0)
					fwrite(buf, n, 1, fps[0]);
			}

			if (pfds[1].revents != 0) {
				while ((n = read(fds[1], buf, WAIT_CHILD_BUF_MAX_SIZE)) > 0)
					fwrite(buf, n, 1, fps[1]);
			}

			sentinel = time(NULL);
		} else if (timeout >= 0 && ((time(NULL) - sentinel) > timeout)) {
			timeouted = 1;
			kill(pid, SIGKILL);
			waitflags = 0;
		}

		if (waitpid(pid, &status, waitflags) == pid)
			break;
	}

	if (status) {
		fprintf(fps[0], "\nERROR: Exit status is %d\n", status);
		if (timeouted)
			fprintf(fps[0], "TIMEOUT: %s\n", ptest_dir);
	}

	return status;
}

int
run_ptests(struct ptest_list *head, const struct ptest_options opts,
		const char *progname, FILE *fp, FILE *fp_stderr)
{
	int rc = 0;
	FILE *xh;

	struct ptest_list *p;
	char stime[GET_STIME_BUF_SIZE];

	pid_t child;
	int pipefd_stdout[2];
	int pipefd_stderr[2];

	if (opts.xml_filename) {
		xh = xml_create(ptest_list_length(head), opts.xml_filename);
		if (!xh)
			exit(EXIT_FAILURE);
	}

	do
	{
		if ((rc = pipe2(pipefd_stdout, O_NONBLOCK)) == -1) 
			break;

		if ((rc = pipe2(pipefd_stderr, O_NONBLOCK)) == -1) {
			close(pipefd_stdout[0]);
			close(pipefd_stdout[1]);
			break;
		}

		fprintf(fp, "START: %s\n", progname);
		PTEST_LIST_ITERATE_START(head, p);
			char *ptest_dir = strdup(p->run_ptest);
			if (ptest_dir == NULL) {
				rc = -1;
				break;
			}
			dirname(ptest_dir);

			child = fork();
			if (child == -1) {
				fprintf(fp, "ERROR: Fork %s\n", strerror(errno));
				rc = -1;
				break;
			} else if (child == 0) {
				run_child(p->run_ptest, pipefd_stdout[1], pipefd_stderr[1]);
			} else {
				int status;
				int fds[2]; fds[0] = pipefd_stdout[0]; fds[1] = pipefd_stderr[0];
				FILE *fps[2]; fps[0] = fp; fps[1] = fp_stderr;

				fprintf(fp, "%s\n", get_stime(stime, GET_STIME_BUF_SIZE));
				fprintf(fp, "BEGIN: %s\n", ptest_dir);

				status = wait_child(ptest_dir, p->run_ptest, child,
						opts.timeout, fds, fps);
				if (status)
					rc += 1;

				if (opts.xml_filename)
					xml_add_case(xh, status, ptest_dir);

				fprintf(fp, "END: %s\n", ptest_dir);
				fprintf(fp, "%s\n", get_stime(stime, GET_STIME_BUF_SIZE));
			}
		PTEST_LIST_ITERATE_END;
		fprintf(fp, "STOP: %s\n", progname);

		close(pipefd_stdout[0]); close(pipefd_stdout[1]);
		close(pipefd_stderr[0]); close(pipefd_stderr[1]);
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
		fprintf(stderr, "XML File could not be created. %s.\n",
				strerror(errno));
		return NULL;
	}

	return xh;
}

void
xml_add_case(FILE *xh, int status, const char *ptest_dir)
{
	fprintf(xh, "\t<testcase classname='%s' name='run-ptest'>\n", ptest_dir);

	if (status != 0) {
		fprintf(xh, "\t\t<failure type='exit_code'");
		fprintf(xh, " message='run-ptest exited with code: %d'>", status);
		fprintf(xh, "</failure>\n");
	}

	fprintf(xh, "\t</testcase>\n");
}

void
xml_finish(FILE *xh)
{
	fprintf(xh, "</testsuite>\n");
	fclose(xh);
}
