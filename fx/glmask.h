#ifndef	__NEMOFX_GL_MASK_H__
#define	__NEMOFX_GL_MASK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct glmask {
	GLuint texture;
	GLuint fbo, dbo;

	GLuint program;

	GLint utexture;
	GLint uoverlay;
	GLint uwidth, uheight;

	int32_t width, height;
};

extern struct glmask *nemofx_glmask_create(int32_t width, int32_t height);
extern void nemofx_glmask_destroy(struct glmask *mask);

extern void nemofx_glmask_resize(struct glmask *mask, int32_t width, int32_t height);
extern void nemofx_glmask_dispatch(struct glmask *mask, uint32_t texture, uint32_t overlay);

static inline int32_t nemofx_glmask_get_width(struct glmask *mask)
{
	return mask->width;
}

static inline int32_t nemofx_glmask_get_height(struct glmask *mask)
{
	return mask->height;
}

static inline uint32_t nemofx_glmask_get_texture(struct glmask *mask)
{
	return mask->texture;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
