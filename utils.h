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

#ifndef _PTEST_RUNNER_UTILS_H_
#define _PTEST_RUNNER_UTILS_H_

#include "ptest_list.h"

#define CHECK_ALLOCATION(p, size, exit_on_null) \
	check_allocation1(p, size, __FILE__, __LINE__, exit_on_null)

extern void check_allocation1(void *, size_t, char *, int, int);
extern struct ptest_list *get_available_ptests(const char *);

#endif
