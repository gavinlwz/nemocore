#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemoshow.h>
#include <talehelper.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

struct showcontext {
	struct eglcanvas *canvas;

	struct nemotale *tale;
	struct nemoshow *show;

	int width, height;

	struct showone *one;
	struct showtransition *trans;
};

static void nemoshow_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct showcontext *context = (struct showcontext *)nemotale_get_userdata(tale);
	struct nemocanvas *canvas = NTEGL_CANVAS(context->canvas);
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (type & NEMOTALE_DOWN_EVENT) {
			struct taletap *taps[16];
			int ntaps;

			ntaps = nemotale_get_node_taps(tale, node, taps, type);
			if (ntaps == 1) {
				nemocanvas_move(canvas, taps[0]->serial);
			} else if (ntaps == 2) {
				nemocanvas_pick(canvas,
						taps[0]->serial,
						taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE));
			}
		} else if (nemotale_is_single_click(tale, event, type) != 0) {
			struct taletap *taps[16];
			int ntaps;

			ntaps = nemotale_get_node_taps(tale, node, taps, type);
			if (ntaps == 1) {
			} else if (ntaps == 2) {
			} else if (ntaps == 3) {
				struct nemotool *tool = nemocanvas_get_tool(canvas);

				nemotool_exit(tool);
			}
		}
	}
}

static void nemoshow_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct showcontext *context = (struct showcontext *)nemotale_get_userdata(tale);

	nemotool_resize_egl_canvas(context->canvas, width, height);
	nemotale_resize_egl(tale, width, height);

	nemotale_composite(tale, NULL);
}

static void nemoshow_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct showcontext *context = (struct showcontext *)nemotale_get_userdata(tale);

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else if (context->trans != NULL) {
		int done;

		done = nemoshow_transition_dispatch(context->trans, secs * 1000 + nsecs / 1000000);
		if (done != 0) {
			context->trans = NULL;
		} else {
			nemocanvas_feedback(canvas);
		}

		nemoshow_one_dirty(context->one);
		nemoshow_update_one(context->show);
		nemoshow_render_one(context->show);
	}

	nemotale_composite(tale, NULL);
}

static void nemoshow_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height, void *userdata)
{
	struct showcontext *context = (struct showcontext *)userdata;
	struct nemotale *tale = context->tale;

	nemotool_resize_egl_canvas(context->canvas, width, height);
	nemotale_resize_egl(tale, width, height);

	nemotale_composite(tale, NULL);
}

static void nemoshow_dispatch_show_composite(struct nemoshow *show, void *userdata)
{
	struct showcontext *context = (struct showcontext *)userdata;
	struct nemotale *tale = context->tale;

	nemocanvas_dispatch_frame(NTEGL_CANVAS(context->canvas));
}

int main(int argc, char *argv[])
{
	struct showcontext *context;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct nemoshow *show;
	struct showtransition *trans;
	int32_t width = 1000, height = 1000;

	context = (struct showcontext *)malloc(sizeof(struct showcontext));
	if (context == NULL)
		return -1;

	context->width = width;
	context->height = height;

	tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	egl = nemotool_create_egl(tool);

	context->canvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), nemoshow_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), nemoshow_dispatch_canvas_frame);

	context->tale = tale = nemotale_create_egl(
			NTEGL_DISPLAY(egl),
			NTEGL_CONTEXT(egl),
			NTEGL_CONFIG(egl));
	nemotale_attach_egl(tale, (EGLNativeWindowType)NTEGL_WINDOW(canvas));
	nemotale_resize_egl(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemoshow_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	nemoshow_initialize();

	context->show = show = nemoshow_create();
	nemoshow_set_tale(show, tale);
	nemoshow_set_dispatch_resize(show, nemoshow_dispatch_show_resize);
	nemoshow_set_dispatch_composite(show, nemoshow_dispatch_show_composite);
	nemoshow_set_userdata(show, context);

	nemoshow_load_xml(show, argv[1]);
	nemoshow_sort_one(show);
	nemoshow_arrange_one(show);
	nemoshow_update_one(show);
	nemoshow_render_one(show);

	nemoshow_set_scene(show,
			nemoshow_search_one(show, "main"));

	context->one = nemoshow_search_one(show, "hour-hand");

	context->trans = trans = nemoshow_transition_create(
			nemoshow_search_one(show, "ease0"),
			3000,
			100);

	nemoshow_transition_attach_sequence(trans,
			nemoshow_search_one(show, "hour-hand-sequence"));

	nemoshow_dispatch_composite(show);

	nemotool_run(tool);

	nemoshow_transition_destroy(trans);

	nemoshow_destroy(show);

	nemoshow_finalize();

	nemotale_destroy(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(context);

	return 0;
}
