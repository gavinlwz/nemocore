#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotale.h>
#include <talenode.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemomisc.h>

void nemotale_destroy(struct nemotale *tale)
{
	tale->destroy(tale);
}

int nemotale_composite(struct nemotale *tale, pixman_region32_t *region)
{
	struct talenode *node;
	int r;

	nemolist_for_each(node, &tale->node_list, link) {
		if (node->transform.dirty != 0) {
			nemotale_damage_below(tale, node);
			nemotale_node_transform_update(node);
			nemotale_damage_below(tale, node);

			node->transform.dirty = 0;
		}

		if (node->dirty != 0) {
			pixman_region32_t damage;

			pixman_region32_init(&damage);

			pixman_region32_intersect(&node->damage, &node->damage, &node->region);

			if (node->transform.enable != 0) {
				pixman_box32_t *extents;

				extents = pixman_region32_extents(&node->damage);
				nemotale_node_boundingbox_update(node,
						extents->x1, extents->y1,
						extents->x2 - extents->x1,
						extents->y2 - extents->y1,
						&damage);
			} else {
				pixman_region32_copy(&damage, &node->damage);
				pixman_region32_translate(&damage,
						node->geometry.x, node->geometry.y);
			}

			pixman_region32_union(&tale->damage, &tale->damage, &damage);

			pixman_region32_fini(&damage);
		}
	}

	r = tale->composite(tale);

	if (region != NULL)
		pixman_region32_union(region, region, &tale->damage);

	nemolist_for_each(node, &tale->node_list, link) {
		if (node->dirty != 0) {
			pixman_region32_clear(&node->damage);

			node->dirty = 0;
		}
	}

	pixman_region32_clear(&tale->damage);

	return r;
}

int nemotale_prepare(struct nemotale *tale)
{
	nemosignal_init(&tale->destroy_signal);

	nemolist_init(&tale->node_list);

	nemolist_init(&tale->ptap_list);
	nemolist_init(&tale->tap_list);
	nemolist_init(&tale->grab_list);

	pixman_region32_init(&tale->damage);

	return 0;
}

void nemotale_finish(struct nemotale *tale)
{
	nemosignal_emit(&tale->destroy_signal, tale);

	pixman_region32_fini(&tale->damage);

	nemolist_remove(&tale->node_list);

	nemolist_remove(&tale->ptap_list);
	nemolist_remove(&tale->tap_list);
	nemolist_remove(&tale->grab_list);
}

struct talenode *nemotale_pick(struct nemotale *tale, float x, float y, float *sx, float *sy)
{
	struct talenode *node;

	nemolist_for_each(node, &tale->node_list, link) {
		if (node->picktype == NEMOTALE_PICK_DEFAULT_TYPE) {
			nemotale_node_transform_from_global(node, x, y, sx, sy);

			if (pixman_region32_contains_point(&node->input, *sx, *sy, NULL))
				return node;
		} else if (node->picktype == NEMOTALE_PICK_CUSTOM_TYPE) {
			nemotale_node_transform_from_global(node, x, y, sx, sy);

			if (node->pick(node, *sx, *sy, node->pickdata) != 0)
				return node;
		}
	}

	return NULL;
}

void nemotale_damage_region(struct nemotale *tale, pixman_region32_t *region)
{
	pixman_region32_union(&tale->damage, &tale->damage, region);
}

void nemotale_damage_below(struct nemotale *tale, struct talenode *node)
{
	pixman_region32_union(&tale->damage, &tale->damage, &node->boundingbox);
}

void nemotale_damage_all(struct nemotale *tale)
{
	pixman_region32_union_rect(&tale->damage, &tale->damage, 0, 0, tale->width, tale->height);
}

void nemotale_attach_node(struct nemotale *tale, struct talenode *node)
{
	nemolist_remove(&node->link);
	nemolist_insert(&tale->node_list, &node->link);

	node->tale = tale;

	nemotale_damage_below(tale, node);
}

void nemotale_detach_node(struct nemotale *tale, struct talenode *node)
{
	nemolist_remove(&node->link);
	nemolist_init(&node->link);

	node->tale = NULL;

	nemotale_damage_below(tale, node);
}
