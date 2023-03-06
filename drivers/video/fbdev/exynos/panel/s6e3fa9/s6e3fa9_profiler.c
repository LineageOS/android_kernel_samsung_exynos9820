/*
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../panel_drv.h"
#include "s6e3fa9_profiler.h"
#include "../display_profiler/maskgen.h"

#ifdef PANEL_PR_TAG
#undef PANEL_PR_TAG
#define PANEL_PR_TAG	"prof"
#endif

void fa9_profile_win_update(struct maptbl *tbl, u8 *dst)
{
	struct profiler_device *profiler = tbl->pdata;

	dst[5] = (u8)((profiler->win_rect.left >> 8) & 0xff);
	dst[6] = (u8)(profiler->win_rect.left & 0xff);

	dst[7] = (u8)((profiler->win_rect.top >> 8) & 0xff);
	dst[8] = (u8)(profiler->win_rect.top & 0xff);

	dst[9] = (u8)((profiler->win_rect.right >> 8) & 0xff);
	dst[10] = (u8)(profiler->win_rect.right & 0xff);

	dst[11] = (u8)(profiler->win_rect.bottom >> 8 & 0xff);
	dst[12] = (u8)(profiler->win_rect.bottom & 0xff);
}

void fa9_profile_display_fps(struct maptbl *tbl, u8 *dst)
{

	unsigned int disp_fps;
	struct profiler_fps *fps;
	struct profiler_device *profiler = tbl->pdata;

	fps = &profiler->fps;

	disp_fps = fps->comp_fps;

	dst[1] = (u8)((disp_fps / 10) & 0xff);
	dst[2] = (u8)((disp_fps % 10) & 0xff);

	panel_info("value : %d, %d:%d\n", disp_fps, dst[1], dst[2]);
}


void fa9_profile_set_color(struct maptbl *tbl, u8 *dst)
{
	struct profiler_device *profiler = tbl->pdata;

	switch(profiler->fps.color) {
		case FPS_COLOR_BLUE:
			dst[2] = 0x0f;
			dst[3] = 0x0f;
			dst[4] = 0xff;
			break;
		case FPS_COLOR_GREEN:
			dst[2] = 0x0f;
			dst[3] = 0xff;
			dst[4] = 0x0f;
			break;
		case FPS_COLOR_RED:
			dst[2] = 0xff;
			dst[3] = 0x0f;
			dst[4] = 0x0f;
			break;
	}
}


void fa9_profile_circle(struct maptbl *tbl, u8 *dst)
{
	struct profiler_device *profiler = tbl->pdata;

	//panel_info("value : %d:%d:%d\n", dst[11], dst[12], dst[13]);
	
	dst[11] = (u8)(profiler->circle_color >> 16) & 0xff;
	dst[12] = (u8)(profiler->circle_color >> 8) & 0xff;
	dst[13] = (u8)profiler->circle_color & 0xff;
}

void fa9_profile_fps_mask_pos(struct maptbl *tbl, u8 *dst)
{
	struct profiler_device *profiler = tbl->pdata;
	struct mprint_props *mask_props;
	struct mprint_config *c;
	
	mask_props = &profiler->mask_props;
	if (!mask_props)
		return;
	
	c = mask_props->conf;
	if (!c)
		return;

	dst[0] = (c->padd_y >> 8) & 0xFF;
	dst[1] = c->padd_y & 0xFF;

	dst[2] = ((c->padd_y + c->resol_y - 1) >> 8) & 0xFF;
	dst[3] = (c->padd_y + c->resol_y - 1) & 0xFF;
}

void fa9_profile_fps_mask_color(struct maptbl *tbl, u8 *dst)
{
	struct profiler_device *profiler = tbl->pdata;
	struct mprint_props *mask_props;
	struct mprint_config *c;
	
	mask_props = &profiler->mask_props;
	if (!mask_props)
		return;
	
	c = mask_props->conf;
	if (!c)
		return;

	dst[0] = (c->color >> 24) & 0xFF;
	dst[1] = (c->color >> 16) & 0xFF;
	dst[2] = (c->color >> 8) & 0xFF;
	dst[3] = c->color & 0xFF;
}

