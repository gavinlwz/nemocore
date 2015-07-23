#ifndef	__NEMO_SHELL_GRAB_H__
#define	__NEMO_SHELL_GRAB_H__

#include <pointer.h>
#include <keyboard.h>
#include <touch.h>

struct shellbin;

struct shellgrab {
	union {
		struct nemopointer_grab pointer;
		struct nemokeyboard_grab keyboard;
		struct touchpoint_grab touchpoint;
	} base;

	struct shellbin *bin;
	struct wl_listener bin_destroy_listener;
	struct wl_listener bin_ungrab_listener;
	struct wl_listener bin_change_listener;
};

struct actorgrab {
	union {
		struct nemopointer_grab pointer;
		struct nemokeyboard_grab keyboard;
		struct touchpoint_grab touchpoint;
	} base;

	struct nemoactor *actor;
	struct wl_listener actor_destroy_listener;
	struct wl_listener actor_ungrab_listener;
};

extern void nemoshell_start_pointer_shellgrab(struct shellgrab *grab, const struct nemopointer_grab_interface *interface, struct shellbin *bin, struct nemopointer *pointer);
extern void nemoshell_end_pointer_shellgrab(struct shellgrab *grab);
extern void nemoshell_start_pointer_actorgrab(struct actorgrab *grab, const struct nemopointer_grab_interface *interface, struct nemoactor *actor, struct nemopointer *pointer);
extern void nemoshell_end_pointer_actorgrab(struct actorgrab *grab);

extern void nemoshell_start_touchpoint_shellgrab(struct shellgrab *grab, const struct touchpoint_grab_interface *interface, struct shellbin *bin, struct touchpoint *tp);
extern void nemoshell_end_touchpoint_shellgrab(struct shellgrab *grab);
extern void nemoshell_start_touchpoint_actorgrab(struct actorgrab *grab, const struct touchpoint_grab_interface *interface, struct nemoactor *actor, struct touchpoint *tp);
extern void nemoshell_end_touchpoint_actorgrab(struct actorgrab *grab);

#endif
