#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>

#include <nemotale.h>
#include <talenode.h>
#include <taleevent.h>
#include <nemomisc.h>

#define	NEMOTALE_CORRECT_EVENT_COORDS(t, x, y)	\
	x *= (t)->viewport.rx; y *= (t)->viewport.ry;

static void taletap_handle_tale_destroy(struct nemolistener *listener, void *data)
{
	struct taletap *tap = (struct taletap *)container_of(listener, struct taletap, tale_destroy_listener);

	nemolist_remove(&tap->link);
	nemolist_init(&tap->link);
	nemolist_remove(&tap->tale_destroy_listener.link);
	nemolist_init(&tap->tale_destroy_listener.link);
}

static void taletap_handle_node_destroy(struct nemolistener *listener, void *data)
{
	struct taletap *tap = (struct taletap *)container_of(listener, struct taletap, node_destroy_listener);

	nemolist_remove(&tap->node_destroy_listener.link);
	nemolist_init(&tap->node_destroy_listener.link);

	tap->node = NULL;
}

static struct taletap *nemotale_create_tap(struct nemotale *tale, uint64_t device)
{
	struct taletap *tap;

	tap = (struct taletap *)malloc(sizeof(struct taletap));
	if (tap == NULL)
		return NULL;

	tap->done = 0;

	tap->device = device;
	tap->node = NULL;

	tap->dist = 0.0f;

	nemolist_init(&tap->link);

	tap->tale_destroy_listener.notify = taletap_handle_tale_destroy;
	nemolist_init(&tap->tale_destroy_listener.link);

	tap->node_destroy_listener.notify = taletap_handle_node_destroy;
	nemolist_init(&tap->node_destroy_listener.link);

	return tap;
}

static void nemotale_destroy_tap(struct taletap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_remove(&tap->tale_destroy_listener.link);
	nemolist_remove(&tap->node_destroy_listener.link);

	free(tap);
}

static void nemotale_remove_tap(struct taletap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_init(&tap->link);
}

void nemotale_push_pointer_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device, float x, float y)
{
	struct taleevent event;
	struct taletap *tap;

	NEMOTALE_CORRECT_EVENT_COORDS(tale, x, y);

	tap = nemotale_pointer_get_tap(tale, device);
	if (tap == NULL) {
		tap = nemotale_create_tap(tale, device);
		if (tap == NULL)
			return;

		nemolist_insert_tail(&tale->ptap_list, &tap->link);
		nemosignal_add(&tale->destroy_signal, &tap->tale_destroy_listener);
	}

	tap->x = x;
	tap->y = y;

	event.type = NEMOTALE_POINTER_ENTER_EVENT;
	event.x = x;
	event.y = y;
	event.device = device;
	event.serial = serial;
	event.tap = tap;

	tale->dispatch_event(tale, NULL, &event);
}

void nemotale_push_pointer_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device)
{
	struct taleevent event;
	struct taletap *tap;

	tap = nemotale_pointer_get_tap(tale, device);
	if (tap == NULL)
		return;

	event.type = NEMOTALE_POINTER_LEAVE_EVENT;
	event.device = device;
	event.serial = serial;
	event.tap = tap;

	tale->dispatch_event(tale, NULL, &event);

	nemotale_destroy_tap(tap);
}

void nemotale_push_pointer_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button)
{
	struct taleevent event;
	struct taletap *tap;
	uint32_t type;
	float sx, sy;

	tap = nemotale_pointer_get_tap(tale, device);
	if (tap == NULL)
		return;

	if (button == BTN_LEFT)
		type = NEMOTALE_POINTER_LEFT_DOWN_EVENT;
	else if (button == BTN_RIGHT)
		type = NEMOTALE_POINTER_RIGHT_DOWN_EVENT;
	else
		type = NEMOTALE_POINTER_BUTTON_DOWN_EVENT;

	tap->node = nemotale_pick_node(tale, tap->x, tap->y, &sx, &sy);
	if (tap->node != NULL) {
		nemolist_remove(&tap->node_destroy_listener.link);

		nemosignal_add(&tap->node->destroy_signal, &tap->node_destroy_listener);
	}

	event.x = tap->x;
	event.y = tap->y;

	tap->grab_time = time;
	tap->grab_value = button;
	tap->grab_x = tap->x;
	tap->grab_y = tap->y;
	tap->serial = serial;

	event.type = type;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.value = button;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);
}

void nemotale_push_pointer_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button)
{
	struct taleevent event;
	struct taletap *tap;
	uint64_t value;
	uint32_t type;

	tap = nemotale_pointer_get_tap(tale, device);
	if (tap == NULL)
		return;

	if (button == BTN_LEFT)
		type = NEMOTALE_POINTER_LEFT_UP_EVENT;
	else if (button == BTN_RIGHT)
		type = NEMOTALE_POINTER_RIGHT_UP_EVENT;
	else
		type = NEMOTALE_POINTER_BUTTON_UP_EVENT;

	event.type = type;
	event.x = tap->x;
	event.y = tap->y;
	event.time = time;
	event.value = button;
	event.device = device;
	event.serial = serial;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);

	if (tap->node != NULL) {
		nemolist_remove(&tap->node_destroy_listener.link);
		nemolist_init(&tap->node_destroy_listener.link);

		tap->node = NULL;
	}
}

void nemotale_push_pointer_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y)
{
	struct taleevent event;
	struct taletap *tap;

	NEMOTALE_CORRECT_EVENT_COORDS(tale, x, y);

	tap = nemotale_pointer_get_tap(tale, device);
	if (tap == NULL)
		return;

	tap->x = x;
	tap->y = y;

	event.type = NEMOTALE_POINTER_MOTION_EVENT;
	event.device = device;
	event.time = time;
	event.x = x;
	event.y = y;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);
}

void nemotale_push_pointer_axis_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t axis, float value)
{
	struct taleevent event;
	struct taletap *tap;

	tap = nemotale_pointer_get_tap(tale, device);
	if (tap == NULL)
		return;

	event.type = NEMOTALE_POINTER_AXIS_EVENT;
	event.device = device;
	event.time = time;
	event.axis = axis;
	event.r = value;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);
}

void nemotale_push_keyboard_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device)
{
	struct taleevent event;

	event.type = NEMOTALE_KEYBOARD_ENTER_EVENT;
	event.device = device;

	tale->dispatch_event(tale, tale->keyboard.focus, &event);
}

void nemotale_push_keyboard_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device)
{
	struct taleevent event;

	event.type = NEMOTALE_KEYBOARD_LEAVE_EVENT;
	event.device = device;

	tale->dispatch_event(tale, tale->keyboard.focus, &event);
}

void nemotale_push_keyboard_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t key)
{
	struct taleevent event;

	event.type = NEMOTALE_KEYBOARD_DOWN_EVENT;
	event.device = device;
	event.value = key;

	tale->dispatch_event(tale, tale->keyboard.focus, &event);
}

void nemotale_push_keyboard_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t key)
{
	struct taleevent event;

	event.type = NEMOTALE_KEYBOARD_UP_EVENT;
	event.device = device;
	event.value = key;

	tale->dispatch_event(tale, tale->keyboard.focus, &event);
}

void nemotale_push_keyboard_layout_event(struct nemotale *tale, uint32_t serial, uint64_t device, const char *name)
{
	struct taleevent event;

	event.type = NEMOTALE_KEYBOARD_LAYOUT_EVENT;
	event.device = device;
	event.name = name;

	tale->dispatch_event(tale, tale->keyboard.focus, &event);
}

void nemotale_push_touch_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy)
{
	struct taleevent event;
	struct taletap *tap;
	uint64_t value;
	float sx, sy;

	NEMOTALE_CORRECT_EVENT_COORDS(tale, x, y);

	tap = nemotale_create_tap(tale, device);
	if (tap == NULL)
		return;

	nemolist_insert_tail(&tale->tap_list, &tap->link);
	nemosignal_add(&tale->destroy_signal, &tap->tale_destroy_listener);

	tap->x = x;
	tap->y = y;
	tap->gx = gx;
	tap->gy = gy;
	tap->serial = serial;

	tap->node = nemotale_pick_node(tale, tap->x, tap->y, &sx, &sy);
	if (tap->node != NULL) {
		nemosignal_add(&tap->node->destroy_signal, &tap->node_destroy_listener);
	}

	tap->grab_time = time;
	tap->grab_x = x;
	tap->grab_y = y;
	tap->grab_gx = gx;
	tap->grab_gy = gy;

	event.type = NEMOTALE_TOUCH_DOWN_EVENT;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.x = x;
	event.y = y;
	event.gx = gx;
	event.gy = gy;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);
}

void nemotale_push_touch_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time)
{
	struct taleevent event;
	struct taletap *tap;

	tap = nemotale_touch_get_tap(tale, device);
	if (tap == NULL)
		return;

	event.type = NEMOTALE_TOUCH_UP_EVENT;
	event.x = tap->x;
	event.y = tap->y;
	event.gx = tap->gx;
	event.gy = tap->gy;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.tap = tap;

	nemotale_remove_tap(tap);

	tale->dispatch_event(tale, tap->node, &event);

	nemotale_destroy_tap(tap);
}

void nemotale_push_touch_motion_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float gx, float gy)
{
	struct taleevent event;
	struct taletap *tap;

	NEMOTALE_CORRECT_EVENT_COORDS(tale, x, y);

	tap = nemotale_touch_get_tap(tale, device);
	if (tap == NULL)
		return;

	tap->dist += sqrtf((gx - tap->gx) * (gx - tap->gx) + (gy - tap->gy) * (gy - tap->gy));

	tap->x = x;
	tap->y = y;
	tap->gx = gx;
	tap->gy = gy;

	event.type = NEMOTALE_TOUCH_MOTION_EVENT;
	event.device = device;
	event.serial = tap->serial;
	event.time = time;
	event.x = x;
	event.y = y;
	event.gx = gx;
	event.gy = gy;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);
}

void nemotale_push_touch_pressure_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float p)
{
	struct taleevent event;
	struct taletap *tap;

	tap = nemotale_touch_get_tap(tale, device);
	if (tap == NULL)
		return;

	event.type = NEMOTALE_TOUCH_PRESSURE_EVENT;
	event.device = device;
	event.serial = tap->serial;
	event.time = time;
	event.p = p;
	event.tap = tap;

	tale->dispatch_event(tale, tap->node, &event);
}

void nemotale_push_timer_event(struct nemotale *tale, uint32_t time)
{
	struct taletap *tap;

	nemolist_for_each(tap, &tale->tap_list, link) {
		if (tap->done != 0)
			continue;

		if (tap->grab_time + tale->long_press_duration <= time) {
			if (tap->dist <= tale->long_press_distance) {
				struct taleevent event;

				event.type = NEMOTALE_TOUCH_LONG_PRESS_EVENT;
				event.device = tap->device;
				event.serial = tap->serial;
				event.time = time;
				event.duration = time - tap->grab_time;
				event.x = tap->x;
				event.y = tap->y;
				event.gx = tap->gx;
				event.gy = tap->gy;
				event.tap = tap;

				tale->dispatch_event(tale, tap->node, &event);
			}
		}
	}
}

void nemotale_push_stick_enter_event(struct nemotale *tale, uint32_t serial, uint64_t device)
{
	struct taleevent event;

	event.type = NEMOTALE_STICK_ENTER_EVENT;
	event.device = device;
	event.serial = serial;

	tale->dispatch_event(tale, NULL, &event);
}

void nemotale_push_stick_leave_event(struct nemotale *tale, uint32_t serial, uint64_t device)
{
	struct taleevent event;

	event.type = NEMOTALE_STICK_LEAVE_EVENT;
	event.device = device;
	event.serial = serial;

	tale->dispatch_event(tale, NULL, &event);
}

void nemotale_push_stick_down_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button)
{
	struct taleevent event;

	event.type = NEMOTALE_STICK_BUTTON_DOWN_EVENT;
	event.device = device;
	event.serial = serial;
	event.time = time;
	event.value = button;

	tale->dispatch_event(tale, NULL, &event);
}

void nemotale_push_stick_up_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, uint32_t button)
{
	struct taleevent event;

	event.type = NEMOTALE_STICK_BUTTON_UP_EVENT;
	event.time = time;
	event.value = button;
	event.device = device;
	event.serial = serial;

	tale->dispatch_event(tale, NULL, &event);
}

void nemotale_push_stick_translate_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float z)
{
	struct taleevent event;

	event.type = NEMOTALE_STICK_TRANSLATE_EVENT;
	event.serial = serial;
	event.device = device;
	event.time = time;
	event.x = x;
	event.y = y;
	event.z = z;

	tale->dispatch_event(tale, NULL, &event);
}

void nemotale_push_stick_rotate_event(struct nemotale *tale, uint32_t serial, uint64_t device, uint32_t time, float x, float y, float z)
{
	struct taleevent event;

	event.type = NEMOTALE_STICK_ROTATE_EVENT;
	event.serial = serial;
	event.device = device;
	event.time = time;
	event.x = x;
	event.y = y;
	event.z = z;

	tale->dispatch_event(tale, NULL, &event);
}
