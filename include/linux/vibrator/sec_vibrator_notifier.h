/*
 *
 *
 * Copyright (C) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __SEC_VIB_NOTIFIER_H__
#define __SEC_VIB_NOTIFIER_H__

#include <linux/notifier.h>

enum SEC_VIB_NOTIFIER {
	SEC_VIB_NOTIFIER_OFF = 0,
	SEC_VIB_NOTIFIER_ON,
};

struct vib_notifier_context {
	int index;
	int timeout;
};

extern int sec_vib_notifier_register(struct notifier_block *n);
extern int sec_vib_notifier_unregister(struct notifier_block *nb);

#endif /* __SEC_VIB_NOTIFIER_H__ */
