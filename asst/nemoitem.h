#ifndef	__NEMO_ITEM_H__
#define	__NEMO_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define	NEMOITEM_ATTR_MAX			(32)

struct itemnode {
	char *name;
	char *attrs[NEMOITEM_ATTR_MAX * 2];
	int nattrs;
};

struct nemoitem {
	struct itemnode *nodes;
	int nnodes;
	int mnodes;
};

#define nemoitem_for_each(item, ivar, iname, ibase)	\
	for (ivar = ibase; (ivar = nemoitem_get(item, iname, ivar)) >= 0; ivar++)

#define nemoitem_for_vattr(item, ivar, fmt, iattr, ibase, attr)	\
	for (iattr = ibase; (attr = nemoitem_get_vattr(item, ivar, fmt, iattr)) != NULL; iattr++)

extern struct nemoitem *nemoitem_create(int mnodes);
extern void nemoitem_destroy(struct nemoitem *item);

extern int nemoitem_set(struct nemoitem *item, const char *name);
extern int nemoitem_get(struct nemoitem *item, const char *name, int index);

extern int nemoitem_get_ifone(struct nemoitem *item, const char *name, int index, const char *attr0, const char *value0);
extern int nemoitem_get_iftwo(struct nemoitem *item, const char *name, int index, const char *attr0, const char *value0, const char *attr1, const char *value1);

extern int nemoitem_set_attr(struct nemoitem *item, int index, const char *attr, const char *value);
extern char *nemoitem_get_attr(struct nemoitem *item, int index, const char *attr);
extern int nemoitem_get_iattr(struct nemoitem *item, int index, const char *attr, int value);
extern float nemoitem_get_fattr(struct nemoitem *item, int index, const char *attr, float value);
extern char *nemoitem_get_vattr(struct nemoitem *item, int index, const char *fmt, ...);

extern void nemoitem_dump(struct nemoitem *item, FILE *out);

static inline int nemoitem_set_attr_named(struct nemoitem *item, const char *name, const char *attr, const char *value)
{
	int index;

	index = nemoitem_get(item, name, 0);
	if (index >= 0) {
		return nemoitem_set_attr(item, index, attr, value);
	}

	return -1;
}

static inline char *nemoitem_get_attr_named(struct nemoitem *item, const char *name, const char *attr)
{
	int index;

	index = nemoitem_get(item, name, 0);
	if (index >= 0) {
		return nemoitem_get_attr(item, index, attr);
	}

	return NULL;
}

static inline int nemoitem_get_iattr_named(struct nemoitem *item, const char *name, const char *attr, int value)
{
	int index;

	index = nemoitem_get(item, name, 0);
	if (index >= 0) {
		return nemoitem_get_iattr(item, index, attr, value);
	}

	return value;
}

static inline float nemoitem_get_fattr_named(struct nemoitem *item, const char *name, const char *attr, float value)
{
	int index;

	index = nemoitem_get(item, name, 0);
	if (index >= 0) {
		return nemoitem_get_fattr(item, index, attr, value);
	}

	return value;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
