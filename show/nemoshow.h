#ifndef	__NEMOSHOW_H__
#define	__NEMOSHOW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>
#include <nemolist.h>
#include <nemolistener.h>

#include <showone.h>
#include <showscene.h>
#include <showcanvas.h>
#include <showitem.h>
#include <showcolor.h>
#include <showsequence.h>
#include <showease.h>
#include <showtransition.h>
#include <showmatrix.h>
#include <showpath.h>
#include <showfilter.h>
#include <showshader.h>
#include <showfont.h>
#include <showeasy.h>
#include <showevent.h>
#include <showgrab.h>
#include <skiahelper.h>

#include <nemotale.h>

#define NEMOSHOW_DEFAULT_TILESIZE		(512)

typedef enum {
	NEMOSHOW_ONTIME_STATE = (1 << 0),
	NEMOSHOW_ANTIALIAS_STATE = (1 << 1),
	NEMOSHOW_FILTER_STATE = (1 << 2)
} NemoShowState;

typedef enum {
	NEMOSHOW_FILTER_LOW_QUALITY = 0,
	NEMOSHOW_FILTER_NORMAL_QUALITY = 1,
	NEMOSHOW_FILTER_HIGH_QUALITY = 2,
	NEMOSHOW_FILTER_LAST_QUALITY
} NemoshowfilterQuality;

typedef enum {
	NEMOSHOW_FRAME_PREPARE_TIME = 0,
	NEMOSHOW_FRAME_RENDER_TIME = 1,
	NEMOSHOW_FRAME_FINISH_TIME = 2,
	NEMOSHOW_LAST_TIME
} NemoShowTime;

struct nemoshow;
struct showone;

typedef void (*nemoshow_enter_frame_t)(struct nemoshow *show, uint32_t msecs);
typedef void (*nemoshow_leave_frame_t)(struct nemoshow *show, uint32_t msecs);

typedef void (*nemoshow_dispatch_event_t)(struct nemoshow *show, struct showevent *event);
typedef void (*nemoshow_dispatch_resize_t)(struct nemoshow *show, int32_t width, int32_t height);
typedef void (*nemoshow_dispatch_transform_t)(struct nemoshow *show, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*nemoshow_dispatch_layer_t)(struct nemoshow *show, int32_t visible);
typedef void (*nemoshow_dispatch_fullscreen_t)(struct nemoshow *show, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);
typedef int (*nemoshow_dispatch_destroy_t)(struct nemoshow *show);

struct showtask {
	struct nemoshow *show;
	struct showone *one;

	int32_t x, y;
	int32_t w, h;
};

struct nemoshow {
	struct nemotale *tale;

	char *name;

	uint32_t state;
	uint32_t quality;

	uint32_t width, height;

	struct nemosignal destroy_signal;

	struct showone *scene;
	struct nemolistener scene_destroy_listener;

	struct nemolist one_list;
	struct nemolist dirty_list;
	struct nemolist bounds_list;
	struct nemolist redraw_list;
	struct nemolist transition_list;

	nemoshow_enter_frame_t enter_frame;
	nemoshow_leave_frame_t leave_frame;
	int frame_depth;

	nemoshow_dispatch_event_t dispatch_event;
	nemoshow_dispatch_resize_t dispatch_resize;
	nemoshow_dispatch_transform_t dispatch_transform;
	nemoshow_dispatch_layer_t dispatch_layer;
	nemoshow_dispatch_fullscreen_t dispatch_fullscreen;
	nemoshow_dispatch_destroy_t dispatch_destroy;

	uint32_t dirty_serial;
	uint32_t transition_serial;

	uint32_t frames;
	uint64_t damages;
	uint32_t times[NEMOSHOW_LAST_TIME];
	uint32_t time0;

	struct nemolist ptap_list;
	struct nemolist tap_list;
	struct nemolist grab_list;

	struct {
		struct showone *focus;

		struct nemolistener one_destroy_listener;
	} keyboard;

	uint32_t single_click_duration;
	uint32_t single_click_distance;

	void *context;
	void *userdata;
};

#define nemoshow_for_each(one, show)	\
	nemolist_for_each(one, &((show)->one_list), link)
#define nemoshow_for_each_reverse(one, show)	\
	nemolist_for_each_reverse(one, &((show)->one_list), link)

extern void nemoshow_initialize(void);
extern void nemoshow_finalize(void);

extern struct nemoshow *nemoshow_create(void);
extern void nemoshow_destroy(struct nemoshow *show);

extern void nemoshow_set_name(struct nemoshow *show, const char *name);

extern struct showone *nemoshow_search_one(struct nemoshow *show, const char *id);

extern void nemoshow_enter_frame(struct nemoshow *show, uint32_t msecs);
extern void nemoshow_leave_frame(struct nemoshow *show, uint32_t msecs);

extern int nemoshow_update_one(struct nemoshow *show);
extern void nemoshow_render_one(struct nemoshow *show);

extern int nemoshow_set_scene(struct nemoshow *show, struct showone *one);
extern void nemoshow_put_scene(struct nemoshow *show);

extern int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height);
extern int nemoshow_set_scale(struct nemoshow *show, double sx, double sy);

extern struct showone *nemoshow_pick_canvas(struct nemoshow *show, float x, float y, float *sx, float *sy);
extern int nemoshow_contain_canvas(struct nemoshow *show, struct showone *one, float x, float y, float *sx, float *sy);

extern void nemoshow_attach_one(struct nemoshow *show, struct showone *one);
extern void nemoshow_detach_one(struct showone *one);

extern void nemoshow_attach_ones(struct nemoshow *show, struct showone *one);
extern void nemoshow_detach_ones(struct showone *one);

extern void nemoshow_attach_transition(struct nemoshow *show, struct showtransition *trans);
extern void nemoshow_detach_transition(struct nemoshow *show, struct showtransition *trans);
extern int nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs);
extern int nemoshow_has_transition(struct nemoshow *show);
extern void nemoshow_ready_transition(struct nemoshow *show, uint32_t msecs);

extern struct showtransition *nemoshow_get_last_transition_one(struct nemoshow *show, struct showone *one, const char *name);
extern struct showtransition *nemoshow_get_last_transition_tag(struct nemoshow *show, uint32_t tag);
extern struct showtransition *nemoshow_get_last_transition_all(struct nemoshow *show);

extern void nemoshow_revoke_transition_one(struct nemoshow *show, struct showone *one, const char *name);
extern void nemoshow_revoke_transition_tag(struct nemoshow *show, uint32_t tag);
extern void nemoshow_revoke_transition_all(struct nemoshow *show);

extern void nemoshow_set_keyboard_focus(struct nemoshow *show, struct showone *one);

extern void nemoshow_enable_antialias(struct nemoshow *show);
extern void nemoshow_disable_antialias(struct nemoshow *show);
extern void nemoshow_enable_filtering(struct nemoshow *show);
extern void nemoshow_disable_filtering(struct nemoshow *show);
extern void nemoshow_set_filtering_quality(struct nemoshow *show, uint32_t quality);

static inline void nemoshow_transform_to_viewport(struct nemoshow *show, float x, float y, float *sx, float *sy)
{
	struct showone *scene = show->scene;

	*sx = x * show->width / NEMOSHOW_SCENE_AT(scene, width);
	*sy = y * show->height / NEMOSHOW_SCENE_AT(scene, height);
}

static inline void nemoshow_transform_from_viewport(struct nemoshow *show, float sx, float sy, float *x, float *y)
{
	struct showone *scene = show->scene;

	*x = sx * NEMOSHOW_SCENE_AT(scene, width) / show->width;
	*y = sy * NEMOSHOW_SCENE_AT(scene, height) / show->height;
}

static inline void nemoshow_set_tale(struct nemoshow *show, struct nemotale *tale)
{
	show->tale = tale;
}

static inline struct nemotale *nemoshow_get_tale(struct nemoshow *show)
{
	return show->tale;
}

static inline void nemoshow_set_context(struct nemoshow *show, void *context)
{
	show->context = context;
}

static inline void *nemoshow_get_context(struct nemoshow *show)
{
	return show->context;
}

static inline void nemoshow_set_userdata(struct nemoshow *show, void *data)
{
	show->userdata = data;
}

static inline void *nemoshow_get_userdata(struct nemoshow *show)
{
	return show->userdata;
}

static inline void nemoshow_set_state(struct nemoshow *show, uint32_t state)
{
	show->state |= state;
}

static inline void nemoshow_put_state(struct nemoshow *show, uint32_t state)
{
	show->state &= ~state;
}

static inline int nemoshow_has_state(struct nemoshow *show, uint32_t state)
{
	return (show->state & state) == state;
}

static inline void nemoshow_set_enter_frame(struct nemoshow *show, nemoshow_enter_frame_t dispatch)
{
	show->enter_frame = dispatch;
}

static inline void nemoshow_set_leave_frame(struct nemoshow *show, nemoshow_leave_frame_t dispatch)
{
	show->leave_frame = dispatch;
}

static inline void nemoshow_set_dispatch_event(struct nemoshow *show, nemoshow_dispatch_event_t dispatch)
{
	show->dispatch_event = dispatch;
}

static inline void nemoshow_set_dispatch_resize(struct nemoshow *show, nemoshow_dispatch_resize_t dispatch)
{
	show->dispatch_resize = dispatch;
}

static inline void nemoshow_set_dispatch_transform(struct nemoshow *show, nemoshow_dispatch_transform_t dispatch)
{
	show->dispatch_transform = dispatch;
}

static inline void nemoshow_set_dispatch_layer(struct nemoshow *show, nemoshow_dispatch_layer_t dispatch)
{
	show->dispatch_layer = dispatch;
}

static inline void nemoshow_set_dispatch_fullscreen(struct nemoshow *show, nemoshow_dispatch_fullscreen_t dispatch)
{
	show->dispatch_fullscreen = dispatch;
}

static inline void nemoshow_set_dispatch_destroy(struct nemoshow *show, nemoshow_dispatch_destroy_t dispatch)
{
	show->dispatch_destroy = dispatch;
}

static inline int nemoshow_make_current(struct nemoshow *show)
{
	return nemotale_make_current(show->tale);
}

static inline void nemoshow_set_single_click_duration(struct nemoshow *show, uint32_t duration)
{
	show->single_click_duration = duration;
}

static inline void nemoshow_set_single_click_distance(struct nemoshow *show, uint32_t distance)
{
	show->single_click_distance = distance;
}

static inline int nemoshow_get_frame_depth(struct nemoshow *show)
{
	return show->frame_depth;
}

#ifdef NEMOSHOW_DEBUG_ON
extern void nemoshow_check_damage(struct nemoshow *show);
extern void nemoshow_dump_times(struct nemoshow *show);

static inline void nemoshow_check_frame(struct nemoshow *show)
{
	show->frames++;
}

static inline void nemoshow_clear_time(struct nemoshow *show)
{
	show->time0 = time_current_msecs();
}

static inline void nemoshow_check_time(struct nemoshow *show, int index)
{
	uint32_t time1 = time_current_msecs();

	show->times[index] += (time1 - show->time0);

	show->time0 = time1;
}
#else
static inline void nemoshow_check_frame(struct nemoshow *show) {}
static inline void nemoshow_check_damage(struct nemoshow *show) {}
static inline void nemoshow_clear_time(struct nemoshow *show) {}
static inline void nemoshow_check_time(struct nemoshow *show, int index) {}
static inline void nemoshow_dump_times(struct nemoshow *show) {}
#endif

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
