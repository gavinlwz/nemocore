#ifndef	__NEMOSHOW_ITEM_H__
#define	__NEMOSHOW_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_ITEM = 0,
	NEMOSHOW_RECT_ITEM = 1,
	NEMOSHOW_RRECT_ITEM = 2,
	NEMOSHOW_CIRCLE_ITEM = 3,
	NEMOSHOW_ARC_ITEM = 4,
	NEMOSHOW_PIE_ITEM = 5,
	NEMOSHOW_TEXT_ITEM = 6,
	NEMOSHOW_PATH_ITEM = 7,
	NEMOSHOW_PATHGROUP_ITEM = 8,
	NEMOSHOW_STYLE_ITEM = 9,
	NEMOSHOW_GROUP_ITEM = 10,
	NEMOSHOW_LAST_ITEM
} NemoShowItemType;

typedef enum {
	NEMOSHOW_NONE_TRANSFORM = 0,
	NEMOSHOW_DIRECT_TRANSFORM = 1,
	NEMOSHOW_INTERN_TRANSFORM = 2,
	NEMOSHOW_EXTERN_TRANSFORM = 3,
	NEMOSHOW_LAST_TRANSFORM
} NemoShowItemTransform;

struct showitem {
	struct showone base;

	struct showone *canvas;
	struct showone *group;

	double x, y;
	double rx, ry;
	double width, height;
	double r;

	double from, to;

	uint32_t stroke;
	double strokes[4];
	double stroke_width;
	uint32_t fill;
	double fills[4];

	double alpha;

	struct showone *style;
	struct showone *blur;
	struct showone *shader;
	struct showone *matrix;
	struct showone *path;
	double length;

	struct showone *font;
	double fontsize;
	double fontascent;
	double fontdescent;
	const char *text;
	double textwidth, textheight;

	int transform;

	void *cc;
};

#define NEMOSHOW_ITEM(one)					((struct showitem *)container_of(one, struct showitem, base))
#define	NEMOSHOW_ITEM_AT(one, at)		(NEMOSHOW_ITEM(one)->at)

extern struct showone *nemoshow_item_create(int type);
extern void nemoshow_item_destroy(struct showone *one);

extern int nemoshow_item_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_item_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_item_update_style(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_child(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_text(struct nemoshow *show, struct showone *one);
extern void nemoshow_item_update_boundingbox(struct nemoshow *show, struct showone *one);

extern void nemoshow_item_set_matrix(struct showone *one, double m[9]);
extern void nemoshow_item_set_shader(struct showone *one, struct showone *shader);

extern double nemoshow_item_get_outer(struct showone *one);

static inline void nemoshow_item_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->fills[0] = r;
	item->fills[1] = g;
	item->fills[2] = b;
	item->fills[3] = a;

	item->fill = 1;
}

static inline void nemoshow_item_set_stroke_color(struct showone *one, double r, double g, double b, double a)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->strokes[0] = r;
	item->strokes[1] = g;
	item->strokes[2] = b;
	item->strokes[3] = a;

	item->stroke = 1;
}

static inline void nemoshow_item_set_stroke_width(struct showone *one, double width)
{
	struct showitem *item = NEMOSHOW_ITEM(one);

	item->stroke_width = width;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
