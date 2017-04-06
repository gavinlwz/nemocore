#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <json.h>

#include <yoyoactor.h>
#include <nemoyoyo.h>
#include <yoyoone.h>
#include <nemomisc.h>

struct yoyoactor *nemoyoyo_actor_create(struct nemoyoyo *yoyo)
{
	struct yoyoactor *actor;

	actor = (struct yoyoactor *)malloc(sizeof(struct yoyoactor));
	if (actor == NULL)
		return NULL;
	memset(actor, 0, sizeof(struct yoyoactor));

	actor->yoyo = yoyo;

	return actor;
}

void nemoyoyo_actor_destroy(struct yoyoactor *actor)
{
	if (actor->icon != NULL)
		nemoyoyo_one_destroy(actor->icon);

	if (actor->timer != NULL)
		nemotimer_destroy(actor->timer);

	free(actor);
}

void nemoyoyo_actor_set_json_object(struct yoyoactor *actor, struct json_object *jobj)
{
	actor->jobj = jobj;
}

static void nemoyoyo_actor_dispatch_transition_done(struct nemotransition *trans, void *data)
{
	struct yoyoactor *actor = (struct yoyoactor *)data;

	nemoyoyo_actor_destroy(actor);
}

static void nemoyoyo_actor_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct yoyoactor *actor = (struct yoyoactor *)data;
	struct nemoyoyo *yoyo = actor->yoyo;
	struct yoyoone *one = actor->icon;
	struct nemotransition *trans;

	trans = nemotransition_create(8,
			NEMOEASE_CUBIC_INOUT_TYPE,
			actor->hidetime,
			0);
	nemoyoyo_one_transition_set_sx(trans, 0, one);
	nemoyoyo_one_transition_set_sy(trans, 1, one);
	nemoyoyo_one_transition_set_alpha(trans, 2, one);
	nemoyoyo_one_transition_check(trans, one);
	nemotransition_set_target(trans, 0, 1.0f, 2.0f);
	nemotransition_set_target(trans, 1, 1.0f, 2.0f);
	nemotransition_set_target(trans, 2, 0.5f, 0.1f);
	nemotransition_set_target(trans, 2, 1.0f, 0.0f);
	nemotransition_set_dispatch_done(trans, nemoyoyo_actor_dispatch_transition_done);
	nemotransition_set_userdata(trans, actor);
	nemotransition_group_attach_transition(yoyo->transitions, trans);

	nemocanvas_dispatch_frame(yoyo->canvas);
}

static int nemoyoyo_actor_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct yoyoone *one = (struct yoyoone *)nemoaction_tap_get_userdata(tap);
	struct yoyoactor *actor = (struct yoyoactor *)nemoyoyo_one_get_userdata(one);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		nemotimer_set_timeout(actor->timer, actor->lifetime);
	} else if (event & NEMOACTION_TAP_MOTION_EVENT) {
		nemotimer_set_timeout(actor->timer, actor->lifetime);
	} else if (event & NEMOACTION_TAP_UP_EVENT) {
		nemoaction_destroy_one_by_target(action, one);
		nemoaction_destroy_tap_by_target(action, one);
		nemoyoyo_actor_destroy(actor);
	}

	return 0;
}

int nemoyoyo_actor_dispatch(struct yoyoactor *actor, struct actiontap *tap)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct yoyoone *one;
	struct cooktex *tex;
	struct json_object *tobj;
	const char *icon = NULL;

	if (json_object_object_get_ex(actor->jobj, "icon", &tobj) != 0)
		icon = json_object_get_string(tobj);
	if (icon == NULL)
		goto err1;

	tex = nemoyoyo_search_tex(yoyo, icon);
	if (tex == NULL)
		goto err1;

	one = actor->icon = nemoyoyo_one_create();
	nemoyoyo_one_set_tx(one,
			nemoaction_tap_get_tx(tap));
	nemoyoyo_one_set_ty(one,
			nemoaction_tap_get_ty(tap));
	nemoyoyo_one_set_width(one,
			nemocook_texture_get_width(tex));
	nemoyoyo_one_set_height(one,
			nemocook_texture_get_height(tex));
	nemoyoyo_one_set_texture(one, tex);
	nemoyoyo_one_set_userdata(one, actor);
	nemoyoyo_attach_one(yoyo, one);

	nemoaction_one_set_tap_callback(yoyo->action, one, nemoyoyo_actor_dispatch_tap_event);

	nemocanvas_dispatch_frame(yoyo->canvas);

	actor->timer = nemotimer_create(nemotool_get_instance());
	nemotimer_set_callback(actor->timer, nemoyoyo_actor_dispatch_timer);
	nemotimer_set_userdata(actor->timer, actor);

	nemotimer_set_timeout(actor->timer, actor->lifetime);

	return 0;

err1:
	nemoyoyo_actor_destroy(actor);

	return -1;
}
