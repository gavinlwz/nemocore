#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <BulletCollision/CollisionShapes/btBox2dShape.h>
#include <BulletCollision/CollisionShapes/btConvex2dShape.h>
#include <BulletCollision/CollisionDispatch/btEmptyCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btBox2dBox2dCollisionAlgorithm.h>
#include <BulletCollision/CollisionDispatch/btConvex2dConvex2dAlgorithm.h>
#include <BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h>

#include <btBulletDynamicsCommon.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <skiahelper.hpp>
#include <nemohelper.h>
#include <nemolist.h>
#include <nemolog.h>
#include <nemomisc.h>

struct physball {
	struct nemolist link;

	btRigidBody *body;

	float color[3];
	float radius;

	uint32_t stime;
	uint32_t etime;
};

struct physcontext {
	struct nemotool *tool;

	int width, height;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *canvas;

	btBroadphaseInterface *broadphase;
	btCollisionDispatcher *dispatcher;
	btConstraintSolver *solver;
	btDefaultCollisionConfiguration *configuration;

	btConvex2dConvex2dAlgorithm::CreateFunc *convex2dalgo;
	btBox2dBox2dCollisionAlgorithm::CreateFunc *box2dalgo;
	btVoronoiSimplexSolver *simplexsolver;
	btMinkowskiPenetrationDepthSolver *pdsolver;

	btDiscreteDynamicsWorld *dynamicsworld;

	btRigidBody *walls[4];

	struct nemolist ball_list;
};

static int nemophys_prepare_wall(struct physcontext *context);
static void nemophys_finish_wall(struct physcontext *context);

static void nemophys_dispatch_canvas_redraw(struct nemoshow *show, struct showone *one)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);
	struct physball *ball, *next;
	uint32_t ctime = time_current_msecs();

	context->dynamicsworld->stepSimulation(1.0f / 60.0f, 0);

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(
				nemoshow_canvas_get_viewport_width(one),
				nemoshow_canvas_get_viewport_height(one),
				kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(nemoshow_canvas_map(one));

	SkBitmapDevice device(bitmap);

	SkCanvas canvas(&device);
	canvas.clear(SK_ColorTRANSPARENT);

	sk_sp<SkMaskFilter> filter = SkBlurMaskFilter::Make(
			kSolid_SkBlurStyle,
			SkBlurMaskFilter::ConvertRadiusToSigma(7.0f),
			SkBlurMaskFilter::kIgnoreTransform_BlurFlag);

	SkPaint paint;
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorWHITE);
	paint.setMaskFilter(filter);

	nemolist_for_each_safe(ball, next, &context->ball_list, link) {
		if (ball->etime > ctime) {
			btRigidBody *body = ball->body;

			if (body != NULL && body->getMotionState()) {
				btTransform transform;

				body->getMotionState()->getWorldTransform(transform);

				paint.setColor(
						SkColorSetARGB(
							255.0f,
							ball->color[0],
							ball->color[1],
							ball->color[2]));

				canvas.drawCircle(
						float(transform.getOrigin().getX()),
						float(transform.getOrigin().getY()),
						ball->radius,
						paint);
			}
		} else {
			nemolist_remove(&ball->link);

			context->dynamicsworld->removeRigidBody(ball->body);

			free(ball);
		}
	}

	nemoshow_canvas_unmap(one);

	nemoshow_one_dirty(one, NEMOSHOW_REDRAW_DIRTY);
}

static void nemophys_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	nemoshow_event_update_taps(show, canvas, event);

	if (nemoshow_event_is_touch_down(show, event)) {
		struct physball *ball;
		float r = random_get_double(0.0f, 255.0f);
		float g = random_get_double(0.0f, 255.0f);
		float b = random_get_double(0.0f, 255.0f);
		float s = random_get_double(15.0f, 45.0f);

		btConvexShape *cicle = new btCylinderShapeZ(btVector3(btScalar(s), btScalar(s), btScalar(s)));
		btConvexShape *shape = new btConvex2dShape(cicle);

		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(
					nemoshow_event_get_x(event) * nemoshow_canvas_get_viewport_sx(canvas),
					nemoshow_event_get_y(event) * nemoshow_canvas_get_viewport_sy(canvas),
					0.0f));

		btScalar mass(1.0f);
		btVector3 linertia(0, 0, 0);

		btDefaultMotionState *motionstate = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo bodyinfo(mass, motionstate, shape, linertia);
		btRigidBody *body = new btRigidBody(bodyinfo);

		context->dynamicsworld->addRigidBody(body);

		ball = (struct physball *)malloc(sizeof(struct physball));
		ball->body = body;
		ball->color[0] = r;
		ball->color[1] = g;
		ball->color[2] = b;
		ball->radius = s;
		ball->stime = time_current_msecs();
		ball->etime = ball->stime + random_get_int(5000, 70000);

		nemolist_insert_tail(&context->ball_list, &ball->link);
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ROTATE_TYPE | NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemophys_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct physcontext *context = (struct physcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	context->width = width;
	context->height = height;

	nemophys_finish_wall(context);
	nemophys_prepare_wall(context);

	nemoshow_view_redraw(context->show);
}

static int nemophys_prepare_wall(struct physcontext *context)
{
	float walls[4][6] = {
		{ context->width / 2.0f, context->height / 2.0f, context->width / 2.0f, context->width / 2.0f, context->height + context->height / 2.0f, 0.0f },
		{ context->width / 2.0f, context->height / 2.0f, context->width / 2.0f, context->width / 2.0f, 0 - context->height / 2.0f, 0.0f },
		{ context->width / 2.0f, context->height / 2.0f, context->width / 2.0f, 0 - context->width / 2.0f, context->height / 2.0f, 0.0f },
		{ context->width / 2.0f, context->height / 2.0f, context->width / 2.0f, context->width + context->width / 2.0f, context->height / 2.0f, 0.0f },
	};
	int i;

	for (i = 0; i < 4; i++) {
		btCollisionShape *shape = new btBoxShape(btVector3(btScalar(walls[i][0]), btScalar(walls[i][1]), btScalar(walls[i][2])));

		btTransform transform;
		transform.setIdentity();
		transform.setOrigin(btVector3(walls[i][3], walls[i][4], walls[i][5]));

		btScalar mass(0.0f);
		btVector3 linertia(0, 0, 0);
		btDefaultMotionState *motionstate = new btDefaultMotionState(transform);
		btRigidBody::btRigidBodyConstructionInfo bodyinfo(mass, motionstate, shape, linertia);
		btRigidBody *body = new btRigidBody(bodyinfo);

		context->dynamicsworld->addRigidBody(body);

		context->walls[i] = body;
	}

	return 0;
}

static void nemophys_finish_wall(struct physcontext *context)
{
	int i;

	for (i = 0; i < 4; i++)
		context->dynamicsworld->removeRigidBody(context->walls[i]);
}

static int nemophys_prepare_bullet(struct physcontext *context)
{
	context->configuration = new btDefaultCollisionConfiguration();
	context->dispatcher = new btCollisionDispatcher(context->configuration);
	context->simplexsolver = new btVoronoiSimplexSolver();
	context->pdsolver = new btMinkowskiPenetrationDepthSolver();

	context->convex2dalgo = new btConvex2dConvex2dAlgorithm::CreateFunc(context->simplexsolver, context->pdsolver);
	context->box2dalgo = new btBox2dBox2dCollisionAlgorithm::CreateFunc();

	context->dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE, CONVEX_2D_SHAPE_PROXYTYPE, context->convex2dalgo);
	context->dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, CONVEX_2D_SHAPE_PROXYTYPE, context->convex2dalgo);
	context->dispatcher->registerCollisionCreateFunc(CONVEX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, context->convex2dalgo);
	context->dispatcher->registerCollisionCreateFunc(BOX_2D_SHAPE_PROXYTYPE, BOX_2D_SHAPE_PROXYTYPE, context->box2dalgo);

	context->broadphase = new btDbvtBroadphase();
	context->solver = new btSequentialImpulseConstraintSolver;

	context->dynamicsworld = new btDiscreteDynamicsWorld(context->dispatcher, context->broadphase, context->solver, context->configuration);
	context->dynamicsworld->setGravity(btVector3(0, 300, 0));

	return 0;
}

static void nemophys_finish_bullet(struct physcontext *context)
{
	delete context->dynamicsworld;

	delete context->solver;

	delete context->broadphase;
	delete context->dispatcher;

	delete context->configuration;

	delete context->convex2dalgo;
	delete context->box2dalgo;

	delete context->simplexsolver;
	delete context->pdsolver;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "fullscreen",			required_argument,			NULL,		'f' },
		{ 0 }
	};

	struct physcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	char *fullscreen = NULL;
	int width = 800;
	int height = 800;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				fullscreen = strdup(optarg);
				break;

			default:
				break;
		}
	}

	context = (struct physcontext *)malloc(sizeof(struct physcontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct physcontext));

	nemolist_init(&context->ball_list);

	context->width = width;
	context->height = height;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemophys_dispatch_show_resize);
	nemoshow_set_userdata(show, context);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemophys_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemophys_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	nemophys_prepare_bullet(context);
	nemophys_prepare_wall(context);

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, context->canvas, NEMOSHOW_FILTER_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemophys_finish_wall(context);
	nemophys_finish_bullet(context);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
