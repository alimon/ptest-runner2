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

#ifndef _PTEST_RUNNER_LIST_H_
#define _PTEST_RUNNER_LIST_H_

#define PTEST_LIST_FREE_CLEAN(x) { ptest_list_free(x); x = NULL; }
#define PTEST_LIST_FREE_ALL_CLEAN(x) { ptest_list_free_all(x); x = NULL; }

#define PTEST_LIST_ITERATE_START(head, p) for (p = head->next; p != NULL; p = p->next) { 
#define PTEST_LIST_ITERATE_END }

struct ptest_list {
	char *ptest;
	char *run_ptest;

	struct ptest_list *next;
	struct ptest_list *prev;
};

extern struct ptest_list *ptest_list_alloc();
extern void ptest_list_free(struct ptest_list *);
extern int ptest_list_free_all(struct ptest_list *);

extern int ptest_list_length(struct ptest_list *);
extern struct ptest_list *ptest_list_search(struct ptest_list *, char *);
extern struct ptest_list *ptest_list_add(struct ptest_list *, char *, char *);
extern struct ptest_list *ptest_list_remove(struct ptest_list *, char *, int);

#endif // _PTEST_RUNNER_LIST_H_
