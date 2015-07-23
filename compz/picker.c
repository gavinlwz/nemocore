#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <picker.h>
#include <compz.h>
#include <view.h>
#include <content.h>
#include <nemomisc.h>

struct nemoview *nemocompz_pick_view(struct nemocompz *compz, float x, float y, float *sx, float *sy)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

#define	NEMOCOMPZ_PICK_VIEW(v, x, y, sx, sy)	\
	nemoview_transform_from_global(v, x, y, sx, sy);	\
	if (v->content->pick == NULL) {	\
		if (pixman_region32_contains_point(&v->content->input, *sx, *sy, NULL)) return v;	\
	} else {	\
		if (v->content->pick(v->content, *sx, *sy)) return v;	\
	}

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					NEMOCOMPZ_PICK_VIEW(child, x, y, sx, sy);
				}
			}

			NEMOCOMPZ_PICK_VIEW(view, x, y, sx, sy);
		}
	}

	return NULL;
}

struct nemoview *nemocompz_pick_canvas(struct nemocompz *compz, float x, float y, float *sx, float *sy)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (view->canvas == NULL)
				continue;

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					NEMOCOMPZ_PICK_VIEW(child, x, y, sx, sy);
				}
			}

			NEMOCOMPZ_PICK_VIEW(view, x, y, sx, sy);
		}
	}

	return NULL;
}
