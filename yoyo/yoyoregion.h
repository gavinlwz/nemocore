#ifndef __NEMOYOYO_REGION_H__
#define __NEMOYOYO_REGION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemoyoyo.h>

typedef enum {
	NEMOYOYO_REGION_RECTANGLE_TYPE = 0,
	NEMOYOYO_REGION_TRIANGLE_TYPE = 1,
	NEMOYOYO_REGION_LAST_TYPE
} NemoYoyoRegionType;

struct yoyoregion {
	int type;

	float x0, y0;
	float x1, y1;
	float x2, y2;

	float rotate;

	struct nemolist link;
};

extern struct yoyoregion *nemoyoyo_region_create(struct nemoyoyo *yoyo);
extern void nemoyoyo_region_destroy(struct yoyoregion *region);

NEMOYOYO_DECLARE_SET_ATTRIBUTE(region, float, rotate, rotate);
NEMOYOYO_DECLARE_GET_ATTRIBUTE(region, float, rotate, rotate);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
