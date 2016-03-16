#ifndef	__NEMOSHOW_SEQUENCE_H__
#define	__NEMOSHOW_SEQUENCE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>

#include <showone.h>

struct nemoshow;
struct showone;

struct showact {
	struct nemolist link;

	struct nemoattr *attr;

	double sattr;
	double eattr;
	double fattr;

	uint32_t offset;
	uint32_t dirty;
	uint32_t state;

	int type;
};

struct showset {
	struct showone base;

	struct showone *src;

	struct nemolist act_list;
	int act_count;
};

struct showframe {
	struct showone base;

	double t;
};

struct showsequence {
	struct showone base;

	double t;

	struct showone *frame;
};

#define NEMOSHOW_SEQUENCE(one)					((struct showsequence *)container_of(one, struct showsequence, base))
#define NEMOSHOW_SEQUENCE_AT(one, at)		(NEMOSHOW_SEQUENCE(one)->at)
#define NEMOSHOW_FRAME(one)							((struct showframe *)container_of(one, struct showframe, base))
#define NEMOSHOW_FRAME_AT(one, at)			(NEMOSHOW_FRAME(one)->at)
#define NEMOSHOW_SET(one)								((struct showset *)container_of(one, struct showset, base))
#define NEMOSHOW_SET_AT(one, at)				(NEMOSHOW_SET(one)->at)

extern struct showone *nemoshow_sequence_create(void);
extern void nemoshow_sequence_destroy(struct showone *one);

extern int nemoshow_sequence_update(struct showone *one);

extern void nemoshow_sequence_prepare(struct showone *one, uint32_t serial);
extern void nemoshow_sequence_dispatch(struct showone *one, double t, uint32_t serial);

extern struct showone *nemoshow_sequence_create_frame(void);
extern void nemoshow_sequence_destroy_frame(struct showone *one);

extern int nemoshow_sequence_update_frame(struct showone *one);

extern int nemoshow_sequence_set_timing(struct showone *one, double t);

extern struct showone *nemoshow_sequence_create_set(void);
extern void nemoshow_sequence_destroy_set(struct showone *one);

extern int nemoshow_sequence_update_set(struct showone *one);

extern int nemoshow_sequence_set_source(struct showone *one, struct showone *src);
extern int nemoshow_sequence_set_attr(struct showone *one, const char *name, const char *value);
extern int nemoshow_sequence_set_dattr(struct showone *one, const char *name, double value);
extern int nemoshow_sequence_set_dattr_offset(struct showone *one, const char *name, int offset, double value);
extern int nemoshow_sequence_set_fattr(struct showone *one, const char *name, double value);
extern int nemoshow_sequence_set_fattr_offset(struct showone *one, const char *name, int offset, double value);
extern int nemoshow_sequence_set_cattr(struct showone *one, const char *name, double r, double g, double b, double a);

extern int nemoshow_sequence_fix_dattr(struct showone *one, int index, double value);

static inline void nemoshow_sequence_set_frame_t(struct showone *one, double t)
{
	NEMOSHOW_FRAME_AT(one, t) = t;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
