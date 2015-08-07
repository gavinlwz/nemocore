#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <nemoxml.h>
#include <nemobox.h>
#include <nemoattr.h>
#include <showmisc.h>
#include <nemomisc.h>
#include <skiaconfig.hpp>

void nemoshow_initialize(void)
{
	SkGraphics::Init();
}

void nemoshow_finalize(void)
{
	SkGraphics::Term();
}

struct nemoshow *nemoshow_create(void)
{
	struct nemoshow *show;

	show = (struct nemoshow *)malloc(sizeof(struct nemoshow));
	if (show == NULL)
		return NULL;
	memset(show, 0, sizeof(struct nemoshow));

	show->expr = nemoshow_expr_create();
	if (show->expr == NULL)
		goto err1;

	show->stable = nemoshow_expr_create_symbol_table();
	if (show->stable == NULL)
		goto err2;

	nemoshow_expr_add_symbol_table(show->expr, show->stable);

	nemolist_init(&show->transition_list);

	show->ones = (struct showone **)malloc(sizeof(struct showone *) * 8);
	show->nones = 0;
	show->sones = 8;

	show->sx = 1.0f;
	show->sy = 1.0f;

	return show;

err2:
	nemoshow_expr_destroy(show->expr);

err1:
	free(show);

	return NULL;
}

void nemoshow_destroy(struct nemoshow *show)
{
	nemoshow_expr_destroy(show->expr);
	nemoshow_expr_destroy_symbol_table(show->stable);

	nemolist_remove(&show->transition_list);

	free(show->ones);
	free(show);
}

struct showone *nemoshow_search_one(struct nemoshow *show, const char *id)
{
	struct showone *one;
	int i;

	if (id == NULL || id[0] == '\0')
		return NULL;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		if (strcmp(one->id, id) == 0)
			return one;
	}

	return NULL;
}

static struct showone *nemoshow_create_one(struct nemoshow *show, struct xmlnode *node)
{
	struct showone *one = NULL;

	if (strcmp(node->name, "scene") == 0) {
		one = nemoshow_scene_create();
	} else if (strcmp(node->name, "canvas") == 0) {
		one = nemoshow_canvas_create();
	} else if (strcmp(node->name, "rect") == 0) {
		one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	} else if (strcmp(node->name, "circle") == 0) {
		one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	} else if (strcmp(node->name, "arc") == 0) {
		one = nemoshow_item_create(NEMOSHOW_ARC_ITEM);
	} else if (strcmp(node->name, "pie") == 0) {
		one = nemoshow_item_create(NEMOSHOW_PIE_ITEM);
	} else if (strcmp(node->name, "text") == 0) {
		one = nemoshow_item_create(NEMOSHOW_TEXT_ITEM);
	} else if (strcmp(node->name, "path") == 0) {
		one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	} else if (strcmp(node->name, "style") == 0) {
		one = nemoshow_item_create(NEMOSHOW_STYLE_ITEM);
	} else if (strcmp(node->name, "loop") == 0) {
		one = nemoshow_loop_create();
	} else if (strcmp(node->name, "sequence") == 0) {
		one = nemoshow_sequence_create();
	} else if (strcmp(node->name, "frame") == 0) {
		one = nemoshow_sequence_create_frame();
	} else if (strcmp(node->name, "set") == 0) {
		one = nemoshow_sequence_create_set();
	} else if (strcmp(node->name, "ease") == 0) {
		one = nemoshow_ease_create();
	} else if (strcmp(node->name, "matrix") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_MATRIX_MATRIX);
	} else if (strcmp(node->name, "scale") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_SCALE_MATRIX);
	} else if (strcmp(node->name, "rotate") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_ROTATE_MATRIX);
	} else if (strcmp(node->name, "translate") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_TRANSLATE_MATRIX);
	} else if (strcmp(node->name, "moveto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_MOVETO_PATH);
	} else if (strcmp(node->name, "lineto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_LINETO_PATH);
	} else if (strcmp(node->name, "curveto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CURVETO_PATH);
	} else if (strcmp(node->name, "close") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CLOSE_PATH);
	} else if (strcmp(node->name, "cmd") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CMD_PATH);
	} else if (strcmp(node->name, "textto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_TEXT_PATH);
	} else if (strcmp(node->name, "camera") == 0) {
		one = nemoshow_camera_create();
	} else if (strcmp(node->name, "blur") == 0) {
		one = nemoshow_blur_create();
	} else if (strcmp(node->name, "linear") == 0) {
		one = nemoshow_shader_create(NEMOSHOW_LINEAR_GRADIENT_SHADER);
	} else if (strcmp(node->name, "radial") == 0) {
		one = nemoshow_shader_create(NEMOSHOW_RADIAL_GRADIENT_SHADER);
	} else if (strcmp(node->name, "stop") == 0) {
		one = nemoshow_stop_create();
	} else if (strcmp(node->name, "var") == 0) {
		one = nemoshow_var_create();
	} else if (strcmp(node->name, "cons") == 0) {
		one = nemoshow_cons_create();
	}

	if (one != NULL) {
		int i;

		for (i = 0; i < node->nattrs; i++) {
			struct showprop *prop;

			prop = nemoshow_get_property(node->attrs[i*2+0]);
			if (prop == NULL)
				continue;

			if (node->attrs[i*2+1][0] == '@') {
				struct showone *vone;

				vone = nemoshow_search_one(show, node->attrs[i*2+1] + 1);
				if (vone != NULL) {
					struct showvar *var = NEMOSHOW_VAR(vone);
					const char *value = nemoobject_gets(&vone->object, "d");

					if (prop->type == NEMOSHOW_STRING_PROP) {
						nemoobject_sets(&one->object, node->attrs[i*2+0], value, strlen(value));
					} else if (prop->type == NEMOSHOW_DOUBLE_PROP) {
						nemoobject_setd(&one->object, node->attrs[i*2+0], strtod(value, NULL));
					} else if (prop->type == NEMOSHOW_INTEGER_PROP) {
						nemoobject_seti(&one->object, node->attrs[i*2+0], strtoul(value, NULL, 10));
					} else if (prop->type == NEMOSHOW_COLOR_PROP) {
						uint32_t c = nemoshow_color_parse(value);
						char attr[NEMOSHOW_ATTR_NAME_MAX];

						snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", node->attrs[i*2+0]);
						nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_R(c));
						snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", node->attrs[i*2+0]);
						nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_G(c));
						snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", node->attrs[i*2+0]);
						nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_B(c));
						snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", node->attrs[i*2+0]);
						nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_A(c));

						nemoobject_seti(&one->object, node->attrs[i*2+0], 1);
					}

					NEMOBOX_APPEND(vone->refs, vone->srefs, vone->nrefs, one);
					NEMOBOX_APPEND(var->refs, var->srefs, var->nrefs,
							nemoshow_var_create_ref(one, node->attrs[i*2+0], prop->type));
				}
			} else if (node->attrs[i*2+1][0] == '!') {
				struct showattr *attr;

				nemoobject_seti(&one->object, node->attrs[i*2+0], 0.0f);

				attr = nemoshow_one_create_attr(
						node->attrs[i*2+0],
						node->attrs[i*2+1] + 1,
						nemoobject_get(&one->object, node->attrs[i*2+0]),
						prop->dirty);

				NEMOBOX_APPEND(one->attrs, one->sattrs, one->nattrs, attr);
			} else {
				if (prop->type == NEMOSHOW_STRING_PROP) {
					nemoobject_sets(&one->object, node->attrs[i*2+0], node->attrs[i*2+1], strlen(node->attrs[i*2+1]));
				} else if (prop->type == NEMOSHOW_DOUBLE_PROP) {
					nemoobject_setd(&one->object, node->attrs[i*2+0], strtod(node->attrs[i*2+1], NULL));
				} else if (prop->type == NEMOSHOW_INTEGER_PROP) {
					nemoobject_seti(&one->object, node->attrs[i*2+0], strtoul(node->attrs[i*2+1], NULL, 10));
				} else if (prop->type == NEMOSHOW_COLOR_PROP) {
					uint32_t c = nemoshow_color_parse(node->attrs[i*2+1]);
					char attr[NEMOSHOW_ATTR_NAME_MAX];

					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_R(c));
					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_G(c));
					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_B(c));
					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_A(c));

					nemoobject_seti(&one->object, node->attrs[i*2+0], 1);
				}
			}
		}
	}

	return one;
}

static int nemoshow_load_one(struct nemoshow *show, struct showone *loop, struct xmlnode *node);
static int nemoshow_load_item(struct nemoshow *show, struct showone *item, struct xmlnode *node);
static int nemoshow_load_loop(struct nemoshow *show, struct showone *item, struct xmlnode *node);
static int nemoshow_load_canvas(struct nemoshow *show, struct showone *canvas, struct xmlnode *node);
static int nemoshow_load_matrix(struct nemoshow *show, struct showone *matrix, struct xmlnode *node);
static int nemoshow_load_scene(struct nemoshow *show, struct showone *scene, struct xmlnode *node);
static int nemoshow_load_frame(struct nemoshow *show, struct showone *frame, struct xmlnode *node);
static int nemoshow_load_sequence(struct nemoshow *show, struct showone *sequence, struct xmlnode *node);
static int nemoshow_load_show(struct nemoshow *show, struct xmlnode *node);

static int nemoshow_load_one(struct nemoshow *show, struct showone *parent, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			NEMOBOX_APPEND(parent->children, parent->schildren, parent->nchildren, one);
			one->parent = parent;
		}
	}

	return 0;
}

static int nemoshow_load_item(struct nemoshow *show, struct showone *item, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_MATRIX_TYPE) {
				nemoshow_load_matrix(show, one, child);
			}

			NEMOBOX_APPEND(item->children, item->schildren, item->nchildren, one);
			one->parent = item;
		}
	}

	return 0;
}

static int nemoshow_load_loop(struct nemoshow *show, struct showone *loop, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_ITEM_TYPE) {
				nemoshow_load_item(show, one, child);
			}

			NEMOBOX_APPEND(loop->children, loop->schildren, loop->nchildren, one);
			one->parent = loop;
		}
	}

	return 0;
}

static int nemoshow_load_canvas(struct nemoshow *show, struct showone *canvas, struct xmlnode *node)
{
	struct showcanvas *cone = NEMOSHOW_CANVAS(canvas);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_LOOP_TYPE) {
				nemoshow_load_loop(show, one, child);
			} else if (one->type == NEMOSHOW_ITEM_TYPE) {
				nemoshow_load_item(show, one, child);
			} else if (one->type == NEMOSHOW_SHADER_TYPE) {
				nemoshow_load_one(show, one, child);
			}

			NEMOBOX_APPEND(canvas->children, canvas->schildren, canvas->nchildren, one);
			one->parent = canvas;
		}
	}

	return 0;
}

static int nemoshow_load_matrix(struct nemoshow *show, struct showone *matrix, struct xmlnode *node)
{
	struct showmatrix *mone = NEMOSHOW_MATRIX(matrix);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->sub == NEMOSHOW_MATRIX_MATRIX) {
				nemoshow_load_matrix(show, one, child);
			}

			NEMOBOX_APPEND(matrix->children, matrix->schildren, matrix->nchildren, one);
			one->parent = matrix;
		}
	}

	return 0;
}

static int nemoshow_load_scene(struct nemoshow *show, struct showone *scene, struct xmlnode *node)
{
	struct showscene *sone = NEMOSHOW_SCENE(scene);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_CANVAS_TYPE) {
				nemoshow_load_canvas(show, one, child);

				one->parent = scene;
			} else if (one->type == NEMOSHOW_MATRIX_TYPE) {
				nemoshow_load_matrix(show, one, child);
			} else if (one->type == NEMOSHOW_CAMERA_TYPE) {
				nemoshow_load_one(show, one, child);
			}

			NEMOBOX_APPEND(scene->children, scene->schildren, scene->nchildren, one);
		}
	}

	return 0;
}

static int nemoshow_load_frame(struct nemoshow *show, struct showone *frame, struct xmlnode *node)
{
	struct showframe *fone = NEMOSHOW_FRAME(frame);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			NEMOBOX_APPEND(frame->children, frame->schildren, frame->nchildren, one);
		}
	}

	return 0;
}

static int nemoshow_load_sequence(struct nemoshow *show, struct showone *sequence, struct xmlnode *node)
{
	struct showsequence *sone = NEMOSHOW_SEQUENCE(sequence);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_FRAME_TYPE) {
				nemoshow_load_frame(show, one, child);
			}

			NEMOBOX_APPEND(sequence->children, sequence->schildren, sequence->nchildren, one);
		}
	}

	return 0;
}

static int nemoshow_load_show(struct nemoshow *show, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_SCENE_TYPE) {
				nemoshow_load_scene(show, one, child);
			} else if (one->type == NEMOSHOW_SEQUENCE_TYPE) {
				nemoshow_load_sequence(show, one, child);
			}
		}
	}

	return 0;
}

int nemoshow_load_xml(struct nemoshow *show, const char *path)
{
	struct nemoxml *xml;
	struct xmlnode *node;

	xml = nemoxml_create();
	nemoxml_load_file(xml, path);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->children, link) {
		if (strcmp(node->name, "show") == 0)
			nemoshow_load_show(show, node);
	}

	nemoxml_destroy(xml);

	return 0;
}

void nemoshow_update_symbol(struct nemoshow *show, const char *name, double value)
{
	nemoshow_expr_add_symbol(show->stable, name, value);
}

void nemoshow_update_expression(struct nemoshow *show)
{
	struct showone *one;
	struct showattr *attr;
	int i, j;

	for (i = 0; i < show->nones; i++) {
		uint32_t dirty = 0x0;

		one = show->ones[i];

		for (j = 0; j < one->nattrs; j++) {
			attr = one->attrs[j];

			nemoattr_setd(attr->ref,
					nemoshow_expr_dispatch_expression(show->expr, attr->text));

			dirty |= attr->dirty;
		}

		nemoshow_one_dirty(one, dirty);
	}
}

void nemoshow_update_one_expression(struct nemoshow *show, struct showone *one)
{
	struct showattr *attr;
	uint32_t dirty = 0x0;
	int i;

	for (i = 0; i < one->nattrs; i++) {
		attr = one->attrs[i];

		nemoattr_setd(attr->ref,
				nemoshow_expr_dispatch_expression(show->expr, attr->text));

		dirty |= attr->dirty;
	}

	for (i = 0; i < one->nchildren; i++) {
		nemoshow_update_one_expression(show, one->children[i]);
	}

	nemoshow_one_dirty(one, dirty);
}

void nemoshow_arrange_one(struct nemoshow *show)
{
	struct showone *one;
	int i;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		if (one->type == NEMOSHOW_SET_TYPE) {
			nemoshow_sequence_arrange_set(show, one);
		} else if (one->type == NEMOSHOW_EASE_TYPE) {
			nemoshow_ease_arrange(show, one);
		} else if (one->type == NEMOSHOW_CANVAS_TYPE && one->sub != NEMOSHOW_CANVAS_SCENE_TYPE) {
			nemoshow_canvas_arrange(show, one);
		} else if (one->type == NEMOSHOW_ITEM_TYPE) {
			nemoshow_item_arrange(show, one);
		} else if (one->type == NEMOSHOW_MATRIX_TYPE) {
			nemoshow_matrix_arrange(show, one);
		} else if (one->type == NEMOSHOW_CAMERA_TYPE) {
			nemoshow_camera_arrange(show, one);
		} else if (one->type == NEMOSHOW_BLUR_TYPE) {
			nemoshow_blur_arrange(show, one);
		} else if (one->type == NEMOSHOW_SHADER_TYPE) {
			nemoshow_shader_arrange(show, one);
		} else if (one->type == NEMOSHOW_LOOP_TYPE) {
			nemoshow_loop_arrange(show, one);
		}
	}

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		if (one->type == NEMOSHOW_CANVAS_TYPE && one->sub == NEMOSHOW_CANVAS_SCENE_TYPE) {
			nemoshow_canvas_arrange(show, one);
		}
	}
}

static inline void nemoshow_update_one_in(struct nemoshow *show, struct showone *one)
{
	if (one->dirty != 0) {
		struct showone *child;
		int i;

		for (i = 0; i < one->nchildren; i++) {
			child = one->children[i];

			if (child->dirty != 0)
				nemoshow_update_one_in(show, child);
		}

		one->update(show, one);

		one->dirty = 0;
	}
}

void nemoshow_update_one(struct nemoshow *show)
{
	struct showone *one;
	int i;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		nemoshow_update_one_in(show, one);
	}
}

void nemoshow_render_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showcanvas *canvas;
	struct showone *one;
	int i;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		nemoshow_one_update(show, one);
	}

	for (i = 0; i < scene->nchildren; i++) {
		one = scene->children[i];

		if (one->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(one);

			if (canvas->needs_redraw != 0) {
				if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
					nemoshow_canvas_render_vector(show, one);
				} else if (one->sub == NEMOSHOW_CANVAS_BACK_TYPE) {
					nemoshow_canvas_render_back(show, one);
				}

				canvas->needs_redraw = 0;
			}
		}
	}

	for (i = 0; i < scene->nchildren; i++) {
		one = scene->children[i];

		if (one->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(one);

			if (canvas->needs_redraw != 0) {
				if (one->type == NEMOSHOW_CANVAS_TYPE) {
					if (one->sub == NEMOSHOW_CANVAS_SCENE_TYPE) {
						nemoshow_canvas_render_scene(show, one);
					}
				}

				canvas->needs_redraw = 0;
			}
		}
	}
}

int nemoshow_set_scene(struct nemoshow *show, struct showone *one)
{
	struct showscene *scene;
	struct showone *child;
	struct showcanvas *canvas;
	int i;

	if (show->scene == one)
		return 0;

	if (show->scene != NULL)
		nemoshow_put_scene(show);

	show->scene = one;

	scene = NEMOSHOW_SCENE(one);

	for (i = 0; i < one->nchildren; i++) {
		child = one->children[i];

		if (child->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(child);

			nemotale_attach_node(show->tale, canvas->node);
		}
	}

	nemotale_resize(show->tale, scene->width, scene->height);

	return 0;
}

void nemoshow_put_scene(struct nemoshow *show)
{
	if (show->scene == NULL)
		return;

	show->scene = NULL;

	nemotale_clear_node(show->tale);
}

int nemoshow_set_camera(struct nemoshow *show, struct showone *one)
{
	if (show->camera == one)
		return 0;

	show->camera = one;

	return 0;
}

void nemoshow_put_camera(struct nemoshow *show)
{
	if (show->camera == NULL)
		return;

	show->camera = NULL;
}

int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height)
{
	struct showone *one;
	struct showone *child;
	int i;

	if (show->scene == NULL)
		return 0;

	if (show->width == width && show->height == height)
		return 0;

	nemotale_set_viewport(show->tale, width, height);

	show->width = width;
	show->height = height;

	one = show->scene;

	for (i = 0; i < one->nchildren; i++) {
		child = one->children[i];

		if (child->type == NEMOSHOW_CANVAS_TYPE) {
			nemoshow_canvas_set_viewport(show, child,
					(double)show->width / (double)NEMOSHOW_SCENE_AT(one, width) * show->sx,
					(double)show->height / (double)NEMOSHOW_SCENE_AT(one, height) * show->sy);
		}
	}

	return 1;
}

int nemoshow_set_scale(struct nemoshow *show, double sx, double sy)
{
	struct showone *one;
	struct showone *child;
	int i;

	if (show->scene == NULL)
		return 0;

	if (show->sx == sx && show->sy == sy)
		return 0;

	show->sx = sx;
	show->sy = sy;

	one = show->scene;

	for (i = 0; i < one->nchildren; i++) {
		child = one->children[i];

		if (child->type == NEMOSHOW_CANVAS_TYPE) {
			nemoshow_canvas_set_viewport(show, child,
					(double)show->width / (double)NEMOSHOW_SCENE_AT(one, width) * show->sx,
					(double)show->height / (double)NEMOSHOW_SCENE_AT(one, height) * show->sy);
		}
	}

	return 1;
}

int nemoshow_attach_canvas(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_attach_node(show->tale, canvas->node);
}

void nemoshow_detach_canvas(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_detach_node(show->tale, canvas->node);
}

void nemoshow_attach_transition(struct nemoshow *show, struct showtransition *trans)
{
	nemolist_insert(&show->transition_list, &trans->link);
}

void nemoshow_detach_transition(struct nemoshow *show, struct showtransition *trans)
{
	nemolist_remove(&trans->link);
}

void nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs)
{
	struct showtransition *trans, *ntrans;
	int done;

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		done = nemoshow_transition_dispatch(trans, msecs);
		if (done != 0) {
			if (trans->callback != NULL)
				trans->callback(trans->userdata);

			nemoshow_transition_destroy(trans);
		}
	}
}

int nemoshow_has_transition(struct nemoshow *show)
{
	return !nemolist_empty(&show->transition_list);
}

void nemoshow_dump_all(struct nemoshow *show, FILE *out)
{
	struct showone *one;
	int i;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		nemoshow_one_dump(one, out);
	}
}
