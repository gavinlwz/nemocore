#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <nemoaction.h>
#include <nemomisc.h>

struct nemoaction *nemoaction_create(void)
{
	struct nemoaction *action;

	action = (struct nemoaction *)malloc(sizeof(struct nemoaction));
	if (action == NULL)
		return NULL;
	memset(action, 0, sizeof(struct nemoaction));

	nemolist_init(&action->one_list);
	nemolist_init(&action->tap_list);

	return action;
}

void nemoaction_destroy(struct nemoaction *action)
{
	nemoaction_destroy_one_all(action);
	nemoaction_destroy_tap_all(action);

	free(action);
}

void nemoaction_destroy_one_all(struct nemoaction *action)
{
	struct actionone *one, *next;

	nemolist_for_each_safe(one, next, &action->one_list, link)
		nemoaction_one_destroy(one);
}

void nemoaction_destroy_one_by_target(struct nemoaction *action, void *target)
{
	struct actionone *one, *next;

	nemolist_for_each_safe(one, next, &action->one_list, link) {
		if (one->target == target)
			nemoaction_one_destroy(one);
	}
}

void nemoaction_destroy_tap_all(struct nemoaction *action)
{
	struct actiontap *tap, *next;

	nemolist_for_each_safe(tap, next, &action->tap_list, link)
		nemoaction_tap_destroy(tap);
}

void nemoaction_destroy_tap_by_target(struct nemoaction *action, void *target)
{
	struct actiontap *tap, *next;

	nemolist_for_each_safe(tap, next, &action->tap_list, link) {
		if (tap->target == target)
			nemoaction_tap_destroy(tap);
	}
}

struct actionone *nemoaction_get_one_by_target(struct nemoaction *action, void *target)
{
	struct actionone *one;

	nemolist_for_each(one, &action->one_list, link) {
		if (one->target == target)
			return one;
	}

	return NULL;
}

struct actiontap *nemoaction_get_tap_by_device(struct nemoaction *action, uint64_t device)
{
	struct actiontap *tap;

	nemolist_for_each(tap, &action->tap_list, link) {
		if (tap->device == device)
			return tap;
	}

	return NULL;
}

struct actiontap *nemoaction_get_tap_by_serial(struct nemoaction *action, uint32_t serial)
{
	struct actiontap *tap;

	nemolist_for_each(tap, &action->tap_list, link) {
		if (tap->serial == serial)
			return tap;
	}

	return NULL;
}

int nemoaction_get_taps_by_target(struct nemoaction *action, void *target, struct actiontap **taps, int mtaps)
{
	struct actiontap *tap;
	int index = 0;

	nemolist_for_each(tap, &action->tap_list, link) {
		if (tap->target == target) {
			taps[index++] = tap;

			if (index >= mtaps)
				break;
		}
	}

	return index;
}

int nemoaction_get_taps_all(struct nemoaction *action, struct actiontap **taps, int mtaps)
{
	struct actiontap *tap;
	int index = 0;

	nemolist_for_each(tap, &action->tap_list, link) {
		taps[index++] = tap;

		if (index >= mtaps)
			break;
	}

	return index;
}

void nemoaction_set_one_tap_callback(struct nemoaction *action, void *target, nemoaction_tap_dispatch_event_t dispatch)
{
	struct actionone *one;

	one = nemoaction_get_one_by_target(action, target);
	if (one == NULL) {
		one = nemoaction_one_create(action);
		one->target = target;
	}

	one->dispatch_tap_event = dispatch;
}
