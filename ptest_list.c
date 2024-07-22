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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * AUTHORS
 * 	Aníbal Limón <anibal.limon@intel.com>
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "utils.h"
#include "ptest_list.h"

#define VALIDATE_PTR_RINT(ptr) \
	do { \
		if (ptr == NULL) { \
			errno = EINVAL; \
			return -1; \
		} \
	} while (0)

#define VALIDATE_PTR_RNULL(ptr) \
	do { \
		if (ptr == NULL) { \
			errno = EINVAL; \
			return NULL; \
		} \
	} while (0)

struct ptest_list *
ptest_list_alloc()
{
	struct ptest_list *p = malloc(sizeof(struct ptest_list));
	CHECK_ALLOCATION(p, sizeof(struct ptest_list), 0);
	if (p != NULL) {
		p->ptest = NULL;
		p->run_ptest = NULL;

		p->next = NULL;
		p->prev = NULL;
	}

	return p;
}

void
ptest_list_free(struct ptest_list *p)
{
	free(p->ptest);
	free(p->run_ptest);
	free(p);
}

int
ptest_list_free_all(struct ptest_list *head)
{
	int i = 0;
	struct ptest_list *p, *q;

	VALIDATE_PTR_RINT(head);

	p = head;
	while (p != NULL) {
		q = p;
		p = p->next;

		ptest_list_free(q);
		i++;
	}

	return i;
}

int
ptest_list_length(struct ptest_list *head)
{
	int i = 0;
	struct ptest_list *p;

	VALIDATE_PTR_RINT(head);

	for (p = head->next; p != NULL; p = p->next)
		i++;

	return i;
}

struct ptest_list *
ptest_list_search(struct ptest_list *head, char *ptest)
{
	struct ptest_list *q = NULL;
	struct ptest_list *p;

	VALIDATE_PTR_RNULL(head);
	VALIDATE_PTR_RNULL(ptest);

	for (p = head; p != NULL; p = p->next) {
		if (p->ptest == NULL) 
			continue;

		if (strcmp(p->ptest, ptest) == 0) {
			q = p;
			break;
		}
	}

	return q;
}


struct ptest_list *
ptest_list_search_by_file(struct ptest_list *head, char *run_ptest, struct stat st_buf)
{
	struct ptest_list *q = NULL;
	struct ptest_list *p;
	struct stat st_buf_p;

	VALIDATE_PTR_RNULL(head);
	VALIDATE_PTR_RNULL(run_ptest);

	for (p = head; p != NULL; p = p->next) {
		if (p->ptest == NULL) 
			continue;

		if (stat(p->run_ptest, &st_buf_p) == -1)
			continue;

		if (strcmp(p->run_ptest, run_ptest) == 0) {
			q = p;
			break;
		}

		/* *
		 * In some ptest packages exists symlink in the ptest directory
		 * causing to load/run twice the same ptest, 
		 *
		 * For example in perl5:
		 * /usr/lib/perl -> /usr/lib/perl5
		 * */
		if (st_buf.st_dev == st_buf_p.st_dev &&
		    st_buf.st_ino == st_buf_p.st_ino) {
			q = p;
			break;
		}
	}

	return q;
}

struct ptest_list *
ptest_list_add(struct ptest_list *head, char *ptest, char *run_ptest)
{
	struct ptest_list *n, *p; 

	VALIDATE_PTR_RNULL(head);
	VALIDATE_PTR_RNULL(ptest);

	n = ptest_list_alloc();
	if (n == NULL)
		return NULL;

	n->ptest = ptest;
	n->run_ptest = run_ptest;

	n->prev = NULL;
	n->next = NULL;

	for (p = head; p->next != NULL; p = p->next);

	n->prev = p;
	p->next = n;

	return n;
}

struct ptest_list *
ptest_list_remove(struct ptest_list *head, char *ptest, int free)
{
	struct ptest_list *p; 
	struct ptest_list *q, *r;

	VALIDATE_PTR_RNULL(head);
	VALIDATE_PTR_RNULL(ptest);

	p = ptest_list_search(head, ptest);

	if (p != NULL) {
		q = p->prev;
		r = p->next;

		if (q != NULL)
			q->next = r;
		if (r != NULL)
			r->prev = q;

		if (free) {
			ptest_list_free(p);
			p = NULL;
		}
	}

	return p;
}

struct ptest_list *
ptest_list_extend(struct ptest_list *head, struct ptest_list *extend)
{
	struct ptest_list *p, *q; 

	VALIDATE_PTR_RNULL(head);
	VALIDATE_PTR_RNULL(extend);

	for (p = head; p->next != NULL; p = p->next);
	q = extend->next;
	p->next = q;

	free(extend);

	return head;
}
