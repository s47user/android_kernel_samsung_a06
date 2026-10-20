// SPDX-License-Identifier: GPL-2.0
/*
 * Boeffla Generic Wakelock Blocker
 *
 * Intercepts kernel wakeup sources and prevents them from waking
 * the CPU or keeping it awake when blocked by sysfs list.
 *
 * Copyright (C) 2014-2018 andip71
 * Adapted for Linux 4.19 / Samsung Galaxy A06
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include "boeffla_wl_blocker.h"

#define MAX_BLOCKED_WLS 64
#define MAX_WL_NAME_LEN 64

struct blocked_wl_entry {
	char name[MAX_WL_NAME_LEN];
	unsigned long blocked_count;
};

static struct blocked_wl_entry blocked_wls[MAX_BLOCKED_WLS];
static int num_blocked_wls = 0;
static DEFINE_SPINLOCK(blocker_lock);

/*
 * is_wl_blocked - Check if a wakelock name is in the blocked list.
 * Safe to be called from interrupt / atomic context.
 */
bool is_wl_blocked(const char *name)
{
	unsigned long flags;
	int i;
	bool blocked = false;

	if (!name || !name[0] || num_blocked_wls == 0)
		return false;

	spin_lock_irqsave(&blocker_lock, flags);
	for (i = 0; i < num_blocked_wls; i++) {
		if (strcmp(blocked_wls[i].name, name) == 0) {
			blocked_wls[i].blocked_count++;
			blocked = true;
			break;
		}
	}
	spin_unlock_irqrestore(&blocker_lock, flags);

	return blocked;
}
EXPORT_SYMBOL_GPL(is_wl_blocked);

static ssize_t wakelock_blocker_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	unsigned long flags;
	ssize_t count = 0;
	int i;

	spin_lock_irqsave(&blocker_lock, flags);
	for (i = 0; i < num_blocked_wls; i++) {
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "%s (%lu)%s",
				   blocked_wls[i].name,
				   blocked_wls[i].blocked_count,
				   (i < num_blocked_wls - 1) ? ";" : "");
	}
	spin_unlock_irqrestore(&blocker_lock, flags);

	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");
	return count;
}

static ssize_t wakelock_blocker_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	unsigned long flags;
	char *dup_buf, *orig_buf, *token;
	int new_count = 0;
	struct blocked_wl_entry new_entries[MAX_BLOCKED_WLS];

	if (count == 0)
		return -EINVAL;

	dup_buf = kstrndup(buf, count, GFP_KERNEL);
	if (!dup_buf)
		return -ENOMEM;

	orig_buf = dup_buf;

	/* Parse tokens separated by ';', ',', or '\n' */
	while ((token = strsep(&dup_buf, ";,\n")) != NULL) {
		token = strim(token);
		if (*token == '\0')
			continue;

		if (new_count >= MAX_BLOCKED_WLS)
			break;

		strlcpy(new_entries[new_count].name, token, MAX_WL_NAME_LEN);
		new_entries[new_count].blocked_count = 0;
		new_count++;
	}

	kfree(orig_buf);

	spin_lock_irqsave(&blocker_lock, flags);
	memcpy(blocked_wls, new_entries, sizeof(new_entries));
	num_blocked_wls = new_count;
	spin_unlock_irqrestore(&blocker_lock, flags);

	pr_info("boeffla_wl_blocker: Updated blocked list with %d wakelocks\n", new_count);
	return count;
}

static DEVICE_ATTR_RW(wakelock_blocker);

static struct attribute *boeffla_wl_blocker_attrs[] = {
	&dev_attr_wakelock_blocker.attr,
	NULL
};
ATTRIBUTE_GROUPS(boeffla_wl_blocker);

static struct miscdevice boeffla_wl_blocker_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "boeffla_wakelock_blocker",
	.groups = boeffla_wl_blocker_groups,
};

static int __init boeffla_wl_blocker_init(void)
{
	int ret;

	ret = misc_register(&boeffla_wl_blocker_dev);
	if (ret) {
		pr_err("boeffla_wl_blocker: Failed to register misc device (ret=%d)\n", ret);
		return ret;
	}

	pr_info("boeffla_wl_blocker: Initialized v1.1.0 successfully\n");
	return 0;
}
late_initcall(boeffla_wl_blocker_init);

MODULE_AUTHOR("andip71 / custom-kernel");
MODULE_DESCRIPTION("Boeffla Generic Wakelock Blocker");
MODULE_LICENSE("GPL v2");
