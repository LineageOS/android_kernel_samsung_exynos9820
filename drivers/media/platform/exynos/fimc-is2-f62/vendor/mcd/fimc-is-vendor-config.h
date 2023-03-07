/*
* Samsung Exynos5 SoC series FIMC-IS driver
 *
 * exynos5 fimc-is vender functions
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_VENDOR_CONFIG_H
#define FIMC_IS_VENDOR_CONFIG_H

#if defined(CONFIG_CAMERA_BEYOND0)
#include "fimc-is-vendor-config_beyond0.h"
#elif defined(CONFIG_CAMERA_BEYOND1)
#include "fimc-is-vendor-config_beyond1.h"
#elif defined(CONFIG_CAMERA_BEYOND2)
#include "fimc-is-vendor-config_beyond2.h"
#elif defined(CONFIG_CAMERA_FMU_V62)
#include "fimc-is-vendor-config_fmu_v62.h"
#else
#include "fimc-is-vendor-config_beyond2.h" /* Default */
#endif

#endif
