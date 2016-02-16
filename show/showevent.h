#ifndef __NEMOSHOW_EVENT_H__
#define __NEMOSHOW_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <taleevent.h>
#include <talegesture.h>
#include <talegrab.h>

typedef int (*nemoshow_grab_dispatch_event_t)(struct nemoshow *show, void *data, uint32_t tag, void *event);

struct showgrab {
	struct talegrab base;

	struct nemoshow *show;

	nemoshow_grab_dispatch_event_t dispatch_event;
	uint32_t tag;
	void *data;

	struct nemolistener destroy_listener;
};

extern struct showgrab *nemoshow_grab_create(struct nemoshow *show, void *event, nemoshow_grab_dispatch_event_t dispatch);
extern void nemoshow_grab_destroy(struct showgrab *grab);

extern void nemoshow_grab_check_signal(struct showgrab *grab, struct nemosignal *signal);

static inline void nemoshow_grab_set_userdata(struct showgrab *grab, void *data)
{
	grab->data = data;
}

static inline void nemoshow_grab_set_tag(struct showgrab *grab, uint32_t tag)
{
	grab->tag = tag;
}

static inline void nemoshow_dispatch_grab(struct nemoshow *show, void *event)
{
	nemotale_dispatch_grab(show->tale, (struct taleevent *)event);
}

static inline float nemoshow_event_get_x(void *event)
{
	return nemotale_event_get_x((struct taleevent *)event);
}

static inline float nemoshow_event_get_y(void *event)
{
	return nemotale_event_get_y((struct taleevent *)event);
}

static inline float nemoshow_event_get_gx(void *event)
{
	return nemotale_event_get_gx((struct taleevent *)event);
}

static inline float nemoshow_event_get_gy(void *event)
{
	return nemotale_event_get_gy((struct taleevent *)event);
}

static inline float nemoshow_event_get_r(void *event)
{
	return nemotale_event_get_r((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_axis(void *event)
{
	return nemotale_event_get_axis((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_serial(void *event)
{
	return nemotale_event_get_serial((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_time(void *event)
{
	return nemotale_event_get_time((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_value(void *event)
{
	return nemotale_event_get_value((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_duration(void *event)
{
	return nemotale_event_get_duration((struct taleevent *)event);
}

static inline uint64_t nemoshow_event_get_device(void *event)
{
	return nemotale_event_get_device((struct taleevent *)event);
}

static inline uint64_t nemoshow_event_get_device_on(void *event, int index)
{
	return nemotale_event_get_device_on((struct taleevent *)event, index);
}

static inline void nemoshow_event_update_taps(struct nemoshow *show, struct showone *one, void *event)
{
	if (one == NULL)
		nemotale_event_update_taps(show->tale, (struct taleevent *)event);
	else
		nemotale_event_update_node_taps(show->tale, NEMOSHOW_CANVAS_AT(one, node), (struct taleevent *)event);
}

static inline void nemoshow_event_get_distant_taps(struct nemoshow *show, void *event, uint64_t *device0, uint64_t *device1)
{
	nemotale_event_get_distant_taps(show->tale, (struct taleevent *)event, device0, device1);
}

static inline int nemoshow_event_is_single_click(struct nemoshow *show, void *event)
{
	return nemotale_event_is_single_click(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_no_tap(struct nemoshow *show, void *event)
{
	return nemotale_event_is_no_tap(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_single_tap(struct nemoshow *show, void *event)
{
	return nemotale_event_is_single_tap(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_double_taps(struct nemoshow *show, void *event)
{
	return nemotale_event_is_double_taps(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_triple_taps(struct nemoshow *show, void *event)
{
	return nemotale_event_is_triple_taps(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_many_taps(struct nemoshow *show, void *event)
{
	return nemotale_event_is_many_taps(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_motion(struct nemoshow *show, void *event)
{
	return nemotale_event_is_motion(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_long_press(struct nemoshow *show, void *event)
{
	return nemotale_event_is_long_press(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_enter(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_enter(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_leave(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_leave(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_button_down(struct nemoshow *show, void *event, uint32_t button)
{
	return nemotale_event_is_pointer_button_down(show->tale, (struct taleevent *)event, button);
}

static inline int nemoshow_event_is_pointer_button_up(struct nemoshow *show, void *event, uint32_t button)
{
	return nemotale_event_is_pointer_button_up(show->tale, (struct taleevent *)event, button);
}

static inline int nemoshow_event_is_pointer_axis(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_axis(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_enter(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_enter(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_leave(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_leave(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_enter(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_enter(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_leave(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_leave(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_translate(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_translate(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_rotate(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_rotate(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_button_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_button_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_button_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_button_up(show->tale, (struct taleevent *)event);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
