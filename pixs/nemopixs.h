#ifndef __NEMOPIXS_H__
#define __NEMOPIXS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>
#include <nemoplay.h>
#include <nemofs.h>
#include <playback.h>

struct pixsfence {
	struct showone *canvas;

	uint8_t *pixels;

	int width, height;
};

struct pixsone {
	float *vertices;
	float *velocities;

	float *diffuses;
	float *noises;
	float *sleeps;

	float *vertices0;
	float *diffuses0;
	float *positions0;
	float *pixels0;
	uint32_t pixscount;
	uint32_t pixscount0;

	GLuint varray;
	GLuint vvertex;
	GLuint vdiffuse;

	int is_vertices_dirty;
	int is_diffuses_dirty;
	int is_texcoords_dirty;
	int is_pixels_dirty;
	int is_hidden;

	struct showone *texture;

	int rows, columns;

	float pixsize;
};

struct nemopixs {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *over;

	struct showone *canvas;

	struct showone *sprites[128];
	int nsprites, isprites;

	struct fsdir *movies;
	int imovies;

	struct showone *video;
	struct nemoplay *play;
	struct playdecoder *decoderback;
	struct playaudio *audioback;
	struct playvideo *videoback;

	int iactions;
	int tapmax;
	int tapidx;

	struct showone *pointsprite;
	struct showone *pointone;

	uint32_t width, height;

	struct nemotimer *stimer;
	uint32_t timeout;

	struct nemotimer *ptimer;

	GLuint fbo, dbo;
	GLuint programs[4];
	GLuint usprite1, usprite3;
	GLuint utexture3;

	struct pixsone *one;

	int pixels;
	float jitter;
	float pixsize;

	uint32_t msecs;

	struct showevent events;
	int has_taps;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
