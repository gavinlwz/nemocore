#ifndef	__NEMO_RENDERER_H__
#define	__NEMO_RENDERER_H__

#include <stdint.h>
#include <pixman.h>

struct nemoscreen;
struct nemocanvas;
struct nemobuffer;
struct nemoactor;
struct rendernode;

struct nemorenderer {
	struct rendernode *node;

	int (*read_pixels)(struct nemorenderer *renderer,
			struct nemoscreen *screen,
			pixman_format_code_t format, void *pixels,
			uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void (*repaint_screen)(struct nemorenderer *renderer,
			struct nemoscreen *screen,
			pixman_region32_t *screen_damage);
	void (*prepare_buffer)(struct nemorenderer *renderer,
			struct nemobuffer *buffer);
	void (*attach_canvas)(struct nemorenderer *renderer,
			struct nemocanvas *canvas);
	void (*flush_canvas)(struct nemorenderer *renderer,
			struct nemocanvas *canvas);
	int (*read_canvas)(struct nemorenderer *renderer,
			struct nemocanvas *canvas,
			pixman_format_code_t format, void *pixels);
	void *(*get_canvas_buffer)(struct nemorenderer *renderer,
			struct nemocanvas *canvas);
	void (*attach_actor)(struct nemorenderer *renderer,
			struct nemoactor *actor);
	void (*flush_actor)(struct nemorenderer *renderer,
			struct nemoactor *actor);
	int (*read_actor)(struct nemorenderer *renderer,
			struct nemoactor *actor,
			pixman_format_code_t format, void *pixels);
	void *(*get_actor_buffer)(struct nemorenderer *renderer,
			struct nemoactor *actor);
	void (*destroy)(struct nemorenderer *renderer);
	void (*make_current)(struct nemorenderer *renderer);
};

struct rendernode {
	struct nemocompz *compz;

	uint32_t nodeid;
	uint32_t id;

	struct nemorenderer *pixman;
	struct nemorenderer *opengl;

	struct wl_list link;
};

extern void rendernode_prepare(struct rendernode *node);
extern void rendernode_finish(struct rendernode *node);

#endif
