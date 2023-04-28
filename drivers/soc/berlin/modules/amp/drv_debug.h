/*
 * Copyright (C) 2012 Marvell Technology Group Ltd.
 *		http://www.marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DRV_DEBUG_H_
#define _DRV_DEBUG_H_

#define AMP_DEVICE_TAG    "[amp_driver] "
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define amp_debug(...)		printk(KERN_DEBUG AMP_DEVICE_TAG __VA_ARGS__)
#else
#define amp_debug(...)
#endif

#define amp_trace(...)		printk(KERN_WARNING AMP_DEVICE_TAG __VA_ARGS__)
#define amp_error(...)		printk(KERN_ERR AMP_DEVICE_TAG __VA_ARGS__)

#endif //_DRV_DEBUG_H_
