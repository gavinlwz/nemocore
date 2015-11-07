#ifndef	__NEMOMOTE_TWEENER_BUILDER_H__
#define	__NEMOMOTE_TWEENER_BUILDER_H__

extern int nemomote_tweener_update(struct nemomote *mote, uint32_t type, double x, double y, double c1[4], double c0[4], double m1, double m0, double s1, double s0);
extern int nemomote_tweener_set(struct nemomote *mote, uint32_t type, double x, double y, double c1[4], double c0[4], double m1, double m0, double s1, double s0);
extern int nemomote_tweener_set_one(struct nemomote *mote, int index, double x, double y, double c1[4], double c0[4], double m1, double m0, double s1, double s0);

#endif
