#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotrans.h>

struct transgroup *nemotrans_group_create(void)
{
	struct transgroup *group;

	group = (struct transgroup *)malloc(sizeof(struct transgroup));
	if (group == NULL)
		return NULL;
	memset(group, 0, sizeof(struct transgroup));

	nemolist_init(&group->list);

	return group;
}

void nemotrans_group_destroy(struct transgroup *group)
{
	free(group);
}

void nemotrans_group_attach_trans(struct transgroup *group, struct nemotrans *trans)
{
	struct nemotrans *strans;
	struct transone *one, *sone, *none;
	int is_first = nemolist_empty(&group->list) != 0;

	nemolist_for_each(one, &trans->list, link) {
		nemolist_for_each(strans, &group->list, link) {
			nemolist_for_each_safe(sone, none, &strans->list, link) {
				if (nemoattr_getp(&one->attr) == nemoattr_getp(&sone->attr)) {
					nemolist_remove(&sone->link);

					free(sone);

					break;
				}
			}
		}
	}

	nemolist_insert_tail(&group->list, &trans->link);

	if (is_first != 0 && group->dispatch_first != NULL)
		group->dispatch_first(group, group->data);
}

void nemotrans_group_detach_trans(struct transgroup *group, struct nemotrans *trans)
{
	nemolist_remove(&trans->link);

	if (nemolist_empty(&group->list) != 0 && group->dispatch_last != NULL)
		group->dispatch_last(group, group->data);
}

void nemotrans_group_set_dispatch_first(struct transgroup *group, nemotrans_group_dispatch_first_t dispatch)
{
	group->dispatch_first = dispatch;
}

void nemotrans_group_set_dispatch_last(struct transgroup *group, nemotrans_group_dispatch_last_t dispatch)
{
	group->dispatch_last = dispatch;
}

void nemotrans_group_set_userdata(struct transgroup *group, void *data)
{
	group->data = data;
}

void nemotrans_group_dispatch(struct transgroup *group, uint32_t msecs)
{
	struct nemotrans *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		if (nemotrans_dispatch(trans, msecs) != 0)
			nemotrans_destroy(trans);
	}

	if (nemolist_empty(&group->list) != 0 && group->dispatch_last != NULL)
		group->dispatch_last(group, group->data);
}

void nemotrans_group_remove_one(struct transgroup *group, void *var)
{
	struct nemotrans *trans;
	struct transone *one, *none;

	nemolist_for_each(trans, &group->list, link) {
		nemolist_for_each_safe(one, none, &trans->list, link) {
			if (nemoattr_getp(&one->attr) == var) {
				nemolist_remove(&one->link);

				free(one);

				break;
			}
		}
	}
}

void nemotrans_group_remove_all(struct transgroup *group)
{
	struct nemotrans *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		nemotrans_destroy(trans);
	}
}

struct nemotrans *nemotrans_create(int type, uint32_t duration, uint32_t delay)
{
	struct nemotrans *trans;

	trans = (struct nemotrans *)malloc(sizeof(struct nemotrans));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct nemotrans));

	nemolist_init(&trans->link);
	nemolist_init(&trans->list);

	nemoease_set(&trans->ease, type);

	trans->duration = duration;
	trans->delay = delay;

	trans->stime = 0;
	trans->etime = 0;

	return trans;
}

void nemotrans_destroy(struct nemotrans *trans)
{
	struct transone *one, *next;

	nemolist_remove(&trans->link);

	nemolist_for_each_safe(one, next, &trans->list, link) {
		nemolist_remove(&one->link);

		free(one);
	}

	free(trans);
}

void nemotrans_ease_set_type(struct nemotrans *trans, int type)
{
	nemoease_set(&trans->ease, type);
}

void nemotrans_ease_set_bezier(struct nemotrans *trans, double x0, double y0, double x1, double y1)
{
	nemoease_set_cubic(&trans->ease, x0, y0, x1, y1);
}

int nemotrans_dispatch(struct nemotrans *trans, uint32_t msecs)
{
	struct transone *one;
	double t;
	int done = 0;

	if (trans->stime == 0) {
		trans->stime = msecs + trans->delay;
		trans->etime = msecs + trans->delay + trans->duration;
	}

	if (trans->stime > msecs)
		return 0;

	if (trans->etime <= msecs) {
		t = 1.0f;
		done = 1;
	} else {
		t = nemoease_get(&trans->ease, msecs - trans->stime, trans->duration);
	}

	nemolist_for_each(one, &trans->list, link) {
		double v = (one->eattr - one->sattr) * t + one->sattr;

		if (one->is_double == 0)
			nemoattr_setf(&one->attr, v);
		else
			nemoattr_setd(&one->attr, v);
	}

	if (trans->dispatch_update != NULL)
		trans->dispatch_update(trans, trans->data);

	return done;
}

void nemotrans_set_float(struct nemotrans *trans, float *var, float value)
{
	struct transone *one;

	one = (struct transone *)malloc(sizeof(struct transone));
	one->sattr = *var;
	one->eattr = value;
	one->is_double = 0;

	nemoattr_setp(&one->attr, var);

	nemolist_insert_tail(&trans->list, &one->link);
}

void nemotrans_set_double(struct nemotrans *trans, double *var, double value)
{
	struct transone *one;

	one = (struct transone *)malloc(sizeof(struct transone));
	one->sattr = *var;
	one->eattr = value;
	one->is_double = 1;

	nemoattr_setp(&one->attr, var);

	nemolist_insert_tail(&trans->list, &one->link);
}

void nemotrans_set_dispatch_update(struct nemotrans *trans, nemotrans_dispatch_update_t dispatch)
{
	trans->dispatch_update = dispatch;
}

void nemotrans_set_userdata(struct nemotrans *trans, void *data)
{
	trans->data = data;
}
