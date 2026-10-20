/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BOEFFLA_WL_BLOCKER_H
#define _BOEFFLA_WL_BLOCKER_H

#include <linux/types.h>

#ifdef CONFIG_BOEFFLA_WL_BLOCKER
bool is_wl_blocked(const char *name);
#else
static inline bool is_wl_blocked(const char *name) { return false; }
#endif

#endif /* _BOEFFLA_WL_BLOCKER_H */
