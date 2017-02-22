#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motztrans.h>
#include <nemomotz.h>
#include <nemomisc.h>

#define NEMOMOTZ_TRANSITION_ONE_TARGETS_MAX			(8)

struct transone {
	float *toattr;
	uint32_t *todirty;

	uint32_t dirty;

	float timings[NEMOMOTZ_TRANSITION_ONE_TARGETS_MAX];
	float targets[NEMOMOTZ_TRANSITION_ONE_TARGETS_MAX];
	int count;
};

struct motztrans *nemomotz_transition_create(int max, int type, uint32_t duration, uint32_t delay)
{
	struct motztrans *trans;

	trans = (struct motztrans *)malloc(sizeof(struct motztrans));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct motztrans));

	trans->ones = (struct transone **)malloc(sizeof(struct transone *) * max);
	if (trans->ones == NULL)
		goto err1;
	memset(trans->ones, 0, sizeof(struct transone *) * max);

	trans->sones = max;
	trans->nones = 0;

	nemolist_init(&trans->link);

	nemoease_set(&trans->ease, type);

	trans->duration = duration;
	trans->delay = delay;

	trans->repeat = 1;

	trans->stime = 0;
	trans->etime = 0;

	return trans;

err1:
	free(trans);

	return NULL;
}

void nemomotz_transition_destroy(struct motztrans *trans)
{
	int i;

	nemolist_remove(&trans->link);

	for (i = 0; i < trans->nones; i++) {
		if (trans->ones[i] != NULL)
			free(trans->ones[i]);
	}

	free(trans);
}

void nemomotz_transition_ease_set_type(struct motztrans *trans, int type)
{
	nemoease_set(&trans->ease, type);
}

void nemomotz_transition_ease_set_bezier(struct motztrans *trans, double x0, double y0, double x1, double y1)
{
	nemoease_set_cubic(&trans->ease, x0, y0, x1, y1);
}

void nemomotz_transition_set_repeat(struct motztrans *trans, uint32_t repeat)
{
	trans->repeat = repeat;
}

int nemomotz_transition_check_repeat(struct motztrans *trans)
{
	if (trans->repeat == 0 || --trans->repeat > 0) {
		trans->stime = 0;
		trans->etime = 0;

		return 0;
	}

	return 1;
}

int nemomotz_transition_set_attr(struct motztrans *trans, int index, float *toattr, float attr, uint32_t *todirty, uint32_t dirty)
{
	struct transone *one;

	trans->ones[index] = one = (struct transone *)malloc(sizeof(struct transone));
	if (one == NULL)
		return -1;
	memset(one, 0, sizeof(struct transone));

	one->count = 0;
	one->toattr = toattr;
	one->todirty = todirty;
	one->dirty = dirty;

	nemomotz_transition_set_target(trans, index, 0.0f, attr);

	trans->nones = MAX(trans->nones, index + 1);

	return 0;
}

void nemomotz_transition_put_attr(struct motztrans *trans, void *var, int size)
{
	struct transone *one;
	int i;

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL && var <= (void *)one->toattr && (void *)one->toattr < var + size) {
			trans->ones[i] = NULL;
			free(one);
		}
	}
}

void nemomotz_transition_set_target(struct motztrans *trans, int index, float t, float v)
{
	struct transone *one = trans->ones[index];
	int i, j;

	for (i = 0; i < one->count; i++) {
		if (t == one->timings[i]) {
			one->targets[i] = v;
			break;
		} else if (t < one->timings[i]) {
			for (j = i; j < one->count; j++) {
				one->timings[j + 1] = one->timings[j];
				one->targets[j + 1] = one->targets[j];
			}

			one->timings[i] = t;
			one->targets[i] = v;
			one->count++;
			break;
		}
	}

	if (i >= one->count) {
		one->timings[one->count] = t;
		one->targets[one->count] = v;
		one->count++;
	}
}

int nemomotz_transition_dispatch(struct motztrans *trans, uint32_t msecs)
{
	struct transone *one;
	float t, u, v;
	int i, j;

	if (trans->stime == 0) {
		trans->stime = msecs + trans->delay;
		trans->etime = msecs + trans->delay + trans->duration;
	}

	if (trans->stime > msecs)
		return 0;

	t = nemoease_get(&trans->ease, msecs - trans->stime, trans->duration);

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL) {
			for (j = 0; j < one->count - 1; j++) {
				if (one->timings[j] <= t && t < one->timings[j + 1]) {
					u = (t - one->timings[j]) / (one->timings[j + 1] - one->timings[j]);
					v = (one->targets[j + 1] - one->targets[j]) * u + one->targets[j];
					break;
				}
			}

			if (j >= one->count - 1)
				v = one->targets[one->count - 1] * (t / one->timings[one->count - 1]);

			*one->toattr = v;
			*one->todirty |= one->dirty;
		}
	}

	return trans->etime <= msecs;
}