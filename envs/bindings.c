#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <signal.h>
#include <linux/input.h>
#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <shell.h>
#include <compz.h>
#include <screen.h>
#include <view.h>
#include <content.h>
#include <actor.h>
#include <canvas.h>
#include <subcanvas.h>
#include <seat.h>
#include <keyboard.h>
#include <pointer.h>
#include <touch.h>
#include <virtuio.h>
#include <datadevice.h>
#include <session.h>
#include <binding.h>
#include <plugin.h>
#include <timer.h>
#include <animation.h>
#include <grab.h>
#include <move.h>
#include <pick.h>
#include <picker.h>
#include <sound.h>
#include <waylandhelper.h>
#include <nemoxml.h>
#include <nemolog.h>
#include <nemoitem.h>
#include <nemobox.h>
#include <nemomisc.h>

#include <nemoenvs.h>
#include <nemotoken.h>
#include <nemohelper.h>

void nemoenvs_handle_escape_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		nemolog_message("SHELL", "exit nemoshell by escape key\n");

		nemocompz_destroy_clients(compz);
		nemocompz_exit(compz);
	}
}

void nemoenvs_handle_left_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data)
{
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (pointer->focus != NULL) {
			nemoview_above_layer(pointer->focus, NULL);

			if (pointer->focus->canvas != NULL) {
				struct nemocanvas *canvas = pointer->focus->canvas;
				struct nemocanvas *parent = nemosubcanvas_get_main_canvas(canvas);
				struct shellbin *bin = nemoshell_get_bin(canvas);

				if (bin != NULL) {
					nemopointer_set_keyboard_focus(pointer, pointer->focus);
					nemoseat_set_stick_focus(pointer->seat, pointer->focus);
					datadevice_set_focus(pointer->seat, pointer->focus);
				}
			} else if (pointer->focus->actor != NULL) {
				nemopointer_set_keyboard_focus(pointer, pointer->focus);
			}
		}

		if (pointer->keyboard != NULL && nemoxkb_has_modifiers_state(pointer->keyboard->xkb, MODIFIER_SHIFT)) {
			if (pointer->focus != NULL && pointer->focus->canvas != NULL) {
				struct nemocanvas *canvas = pointer->focus->canvas;
				pixman_image_t *image;

				image = pixman_image_create_bits(PIXMAN_a8r8g8b8, canvas->base.width, canvas->base.height, NULL, canvas->base.width * 4);

				nemocontent_read_pixels(&canvas->base, PIXMAN_a8r8g8b8, pixman_image_get_data(image));

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
			} else if (pointer->focus != NULL && pointer->focus->actor != NULL) {
				struct nemoactor *actor = pointer->focus->actor;
				pixman_image_t *image;

				image = pixman_image_create_bits(PIXMAN_a8r8g8b8, actor->base.width, actor->base.height, NULL, actor->base.width * 4);

				nemocontent_read_pixels(&actor->base, PIXMAN_a8r8g8b8, pixman_image_get_data(image));

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
			}
		} else if (pointer->keyboard != NULL && nemoxkb_has_modifiers_state(pointer->keyboard->xkb, MODIFIER_CTRL)) {
			struct nemoscreen *screen;

			screen = nemocompz_get_screen_on(compz, pointer->x, pointer->y);
			if (screen != NULL) {
				pixman_image_t *image;

				image = pixman_image_create_bits(PIXMAN_a8b8g8r8, screen->width, screen->height, NULL, screen->width * 4);

				nemoscreen_read_pixels(screen, PIXMAN_a8b8g8r8,
						pixman_image_get_data(image),
						screen->x, screen->y,
						screen->width, screen->height);

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
			}
		}

		wl_signal_emit(&compz->activate_signal, pointer->focus);
	}
}

static void nemoenvs_handle_touch_binding(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)data;
	struct nemoscreen *screen;

	screen = nemocompz_get_screen_on(compz, pointer->x, pointer->y);

	nemoinput_set_screen(tp->touch->node, screen);

	nemolog_message("SHELL", "bind touch(%s) to screen(%d:%d)\n", tp->touch->node->devnode, screen->node->nodeid, screen->screenid);
}

static void nemoenvs_handle_key_binding(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		struct nemopointer *pointer = (struct nemopointer *)data;

		nemopointer_set_keyboard(pointer, keyboard);

		nemolog_message("SHELL", "bind keyboard(%s) to pointer(%s)\n", keyboard->node->devnode, pointer->node->devnode);
	}
}

void nemoenvs_handle_right_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data)
{
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		pointer->bindings[0] = nemocompz_add_key_binding(compz, KEY_ENTER, 0, nemoenvs_handle_key_binding, (void *)pointer);
	} else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (pointer->bindings[0] != NULL) {
			nemobinding_destroy(pointer->bindings[0]);

			pointer->bindings[0] = NULL;
		}
	}
}

void nemoenvs_handle_touch_event(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	if (tp->focus != NULL) {
		nemoview_above_layer(tp->focus, NULL);

		if (tp->focus->canvas != NULL) {
			struct nemocanvas *parent = nemosubcanvas_get_main_canvas(tp->focus->canvas);
			struct shellbin *bin = nemoshell_get_bin(tp->focus->canvas);

			if (bin != NULL) {
				nemoseat_set_stick_focus(tp->touch->seat, tp->focus);
				datadevice_set_focus(tp->touch->seat, tp->focus);

				if ((bin->flags & NEMOSHELL_SURFACE_BINDABLE_FLAG) && (bin->fixed == 0)) {
					struct touchpoint *tps[10];
					int tapcount;

					tapcount = nemoseat_get_touchpoint_by_view(compz->seat, tp->focus, tps, 10);
					if (tapcount >= 3) {
						struct touchpoint *tp0, *tp1;

						nemoseat_get_distant_touchpoint(compz->seat, tps, tapcount, &tp0, &tp1);

						if (bin->flags & NEMOSHELL_SURFACE_RESIZABLE_FLAG) {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE), bin);
						} else if (bin->flags & NEMOSHELL_SURFACE_SCALABLE_FLAG) {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALEONLY), bin);
						} else {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE), bin);
						}
					} else if (bin->grabbed > 0 && tapcount == 2) {
						struct touchpoint *tp0 = tps[0];
						struct touchpoint *tp1 = tps[1];

						if (bin->flags & NEMOSHELL_SURFACE_RESIZABLE_FLAG) {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE), bin);
						} else if (bin->flags & NEMOSHELL_SURFACE_SCALABLE_FLAG) {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALEONLY), bin);
						} else {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE), bin);
						}
					} else if (bin->grabbed > 0 && tapcount == 1) {
						nemoshell_move_canvas_by_touchpoint(bin->shell, tps[0], bin);
					}
				}
			}
		}
	}

	wl_signal_emit(&compz->activate_signal, tp->focus);
}