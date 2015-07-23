#ifndef	__NEMO_ANIMATION_H__
#define	__NEMO_ANIMATION_H__

#include <stdint.h>

#include <nemoease.h>

typedef enum {
	NEMO_VIEW_ANIMATION_EASE_TYPE = 0,
	NEMO_VIEW_ANIMATION_CUBIC_TYPE = 1,
	NEMO_VIEW_ANIMATION_LAST_TYPE
} NemoViewAnimationEaseType;

struct nemoanimation;

typedef void (*nemoanimation_frame_t)(struct nemoanimation *animation, double progress);
typedef void (*nemoanimation_done_t)(struct nemoanimation *animation);

struct nemoanimation {
	struct wl_list link;

	uint32_t frame_count;

	int type;

	uint32_t stime, etime;
	uint32_t delay;
	uint32_t duration;

	struct nemoease ease;

	nemoanimation_frame_t frame;
	nemoanimation_done_t done;
};

#endif
