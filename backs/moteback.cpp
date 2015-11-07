#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <getopt.h>

#define	SK_RELEASE			1
#define	SK_CPU_LENDIAN	1
#define SK_R32_SHIFT		16
#define SK_G32_SHIFT		8
#define SK_B32_SHIFT		0
#define SK_A32_SHIFT		24

#include <SkTypes.h>
#include <SkCanvas.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkImageEncoder.h>
#include <SkImageDecoder.h>
#include <SkBitmapDevice.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkString.h>
#include <SkMatrix.h>
#include <SkRegion.h>
#include <SkParsePath.h>
#include <SkTypeface.h>

#include <SkBlurMask.h>
#include <SkBlurMaskFilter.h>
#include <SkEmbossMaskFilter.h>

#include <SkGradientShader.h>

#include <SkGeometry.h>

#include <nemomote.h>
#include <nemotool.h>
#include <nemotimer.h>
#include <nemoegl.h>
#include <pixmanhelper.h>
#include <colorhelper.h>
#include <talehelper.h>
#include <nemomisc.h>

struct moteback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *back;
	struct talenode *node;

	int32_t width, height;

	struct nemomote *mote;
	struct moterandom random;
	struct nemozone box;
	struct nemozone disc;
	struct nemozone speed;

	struct nemoease ease;

	double secs;

	struct nemotimer *timer;

	int type;
	char *logo;
	double pixelsize;

	double colors0[4];
	double colors1[4];
	double tcolors0[4];
	double tcolors1[4];
};

static void moteback_prepare_text(struct moteback *mote, const char *text, int ntext)
{
	double pixelsize = mote->pixelsize;
	double x, y;
	int size = 16;
	int fontsize = 16;
	int i, j, p;

	SkBitmap bitmap;
	bitmap.allocPixels(SkImageInfo::Make(size * ntext, size, kN32_SkColorType, kPremul_SkAlphaType));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);

	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorWHITE);
	paint.setTypeface(SkTypeface::CreateFromFile("/usr/share/fonts/ttf/LiberationMono-Regular.ttf", 0));
	paint.setTextSize(fontsize);

	SkPaint::FontMetrics metrics;
	paint.getFontMetrics(&metrics, 0);

	canvas.drawText(
			text, ntext,
			SkIntToScalar(0), SkIntToScalar(-metrics.fAscent),
			paint);

	SkScalar widths[ntext];
	double width = 0.0f;
	int count;

	count = paint.getTextWidths(text, ntext, widths, NULL);

	for (i = 0; i < count; i++) {
		width += ceil(widths[i]);
	}

	x = random_get_double(0.0f, mote->width - width * pixelsize);
	y = random_get_double(0.0f, mote->height - size * pixelsize);

	for (i = 0; i < size; i++) {
		for (j = 0; j < size * ntext; j++) {
			if (bitmap.getColor(j, i) != SK_ColorTRANSPARENT) {
				p = nemomote_get_one_by_type(mote->mote, 1);
				if (p < 0)
					return;

				nemomote_tweener_set_one(mote->mote, p,
						x + j * pixelsize,
						y + i * pixelsize,
						mote->tcolors1, mote->tcolors0,
						pixelsize * 0.5f, pixelsize * 0.3f,
						5.0f, 1.0f);
				nemomote_type_set_one(mote->mote, p, 5);
			}
		}
	}
}

static void moteback_update_one(struct moteback *mote, double secs)
{
	nemomote_mutualgravity_update(mote->mote, 1, secs, 100.0f, 5000.0f, 50.0f);
	nemomote_speedlimit_update(mote->mote, 2, secs, 0.0f, 50.0f);
	nemomote_collide_update(mote->mote, 2, 1, secs, 1.5f);
	nemomote_collide_update(mote->mote, 2, 2, secs, 1.5f);
	nemomote_speedlimit_update(mote->mote, 1, secs, 0.0f, 300.0f);

	if (nemomote_tween_update(mote->mote, 5, secs, &mote->ease, 6, NEMOMOTE_POSITION_TWEEN | NEMOMOTE_COLOR_TWEEN | NEMOMOTE_MASS_TWEEN) != 0) {
		nemomote_tweener_set(mote->mote, 6,
				0.0f, 0.0f,
				mote->colors1, mote->colors0,
				8.0f, 3.0f,
				5.0f, 1.0f);
	}

	if (nemomote_tween_update(mote->mote, 6, secs, &mote->ease, 7, NEMOMOTE_COLOR_TWEEN | NEMOMOTE_MASS_TWEEN) != 0) {
		nemomote_explosion_update(mote->mote, 7, secs, -30.0f, 30.0f, -30.0f, 30.0f);
		nemomote_sleeptime_set(mote->mote, 7, 9.0f, 3.0f);
		nemomote_type_set(mote->mote, 7, 1);
	}

	nemomote_boundingbox_update(mote->mote, 2, secs, &mote->box, 0.8f);
	nemomote_move_update(mote->mote, 2, secs);
	nemomote_boundingbox_update(mote->mote, 1, secs, &mote->box, 0.8f);
	nemomote_move_update(mote->mote, 1, secs);

	nemomote_cleanup(mote->mote);
}

static void moteback_render_back(struct moteback *mote, pixman_image_t *image, const char *uri)
{
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);

	SkBitmap back;
	SkImageDecoder::DecodeFile(uri, &back);

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.drawBitmapRect(back,
			SkRect::MakeXYWH(0, 0, width, height));
}

static void moteback_render_one(struct moteback *mote, pixman_image_t *image)
{
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);
	int i;

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);

	SkMaskFilter *dfilter = SkBlurMaskFilter::Create(
			kSolid_SkBlurStyle,
			SkBlurMask::ConvertRadiusToSigma(5.0f),
			SkBlurMaskFilter::kHighQuality_BlurFlag);

	SkMaskFilter *sfilter = SkBlurMaskFilter::Create(
			kOuter_SkBlurStyle,
			SkBlurMask::ConvertRadiusToSigma(5.0f),
			SkBlurMaskFilter::kHighQuality_BlurFlag);

	SkPaint dfill;
	dfill.setAntiAlias(true);
	dfill.setStyle(SkPaint::kFill_Style);
	dfill.setMaskFilter(dfilter);

	SkPaint sfill;
	sfill.setAntiAlias(true);
	sfill.setStyle(SkPaint::kFill_Style);
	sfill.setMaskFilter(sfilter);

	for (i = 0; i < nemomote_get_count(mote->mote); i++) {
		canvas.save();

		canvas.clipRect(
				SkRect::MakeXYWH(
					MAX(NEMOMOTE_POSITION_X(mote->mote, i) - NEMOMOTE_MASS(mote->mote, i) - 8.0f, 0.0f),
					MAX(NEMOMOTE_POSITION_Y(mote->mote, i) - NEMOMOTE_MASS(mote->mote, i) - 8.0f, 0.0f),
					(NEMOMOTE_MASS(mote->mote, i) + 8.0f) * 2.0f,
					(NEMOMOTE_MASS(mote->mote, i) + 8.0f) * 2.0f));

		if (NEMOMOTE_TYPE(mote->mote, i) != 2) {
			dfill.setColor(
					SkColorSetARGB(
						NEMOMOTE_COLOR_A(mote->mote, i) * 255.0f,
						NEMOMOTE_COLOR_R(mote->mote, i) * 255.0f,
						NEMOMOTE_COLOR_G(mote->mote, i) * 255.0f,
						NEMOMOTE_COLOR_B(mote->mote, i) * 255.0f));

			canvas.drawCircle(
					NEMOMOTE_POSITION_X(mote->mote, i),
					NEMOMOTE_POSITION_Y(mote->mote, i),
					NEMOMOTE_MASS(mote->mote, i),
					dfill);
		} else {
			sfill.setColor(
					SkColorSetARGB(
						NEMOMOTE_COLOR_A(mote->mote, i) * 255.0f,
						NEMOMOTE_COLOR_R(mote->mote, i) * 255.0f,
						NEMOMOTE_COLOR_G(mote->mote, i) * 255.0f,
						NEMOMOTE_COLOR_B(mote->mote, i) * 255.0f));

			canvas.drawCircle(
					NEMOMOTE_POSITION_X(mote->mote, i),
					NEMOMOTE_POSITION_Y(mote->mote, i),
					NEMOMOTE_MASS(mote->mote, i),
					sfill);
		}

		canvas.restore();
	}
}

static void moteback_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct moteback *mote = (struct moteback *)nemotale_get_userdata(tale);
	struct talenode *node = mote->node;

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else {
		nemocanvas_feedback(canvas);

		if (mote->secs == 0.0f)
			moteback_update_one(mote, 0.0f);
		else
			moteback_update_one(mote, ((double)secs + (double)nsecs / 1000000000) - mote->secs);

		mote->secs = (double)secs + (double)nsecs / 1000000000;
	}

	moteback_render_one(mote, nemotale_node_get_pixman(node));

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);
}

static void moteback_dispatch_timer_event(struct nemotimer *timer, void *data)
{
	struct moteback *mote = (struct moteback *)data;
	char msg[64];

	if (mote->type == 0) {
		time_t tt;
		struct tm *tm;

		time(&tt);
		tm = localtime(&tt);

		strftime(msg, sizeof(msg), "%m:%d-%I:%M", tm);

		moteback_prepare_text(mote, msg, strlen(msg));
	} else {
		strcpy(msg, mote->logo);

		moteback_prepare_text(mote, msg, strlen(msg));
	}

	mote->type = (mote->type + 1) % 2;

	nemotimer_set_timeout(mote->timer, 30 * 1000);
}

static void moteback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,			NULL,		'w' },
		{ "height",				required_argument,			NULL,		'h' },
		{ "uri",					required_argument,			NULL,		'u' },
		{ "logo",					required_argument,			NULL,		'l' },
		{ "pixelsize",		required_argument,			NULL,		's' },
		{ "color",				required_argument,			NULL,		'c' },
		{ "textcolor",		required_argument,			NULL,		't' },
		{ 0 }
	};
	struct moteback *mote;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	int32_t width = 1920;
	int32_t height = 1080;
	double pixelsize = 18.0f;
	double color[4] = { 0.0f, 0.3f, 0.3f, 0.0f };
	double textcolor[4] = { 0.0f, 1.0f, 1.0f, 0.0f };
	uint32_t uc;
	char *uri = NULL;
	char *logo = NULL;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "w:h:u:l:s:c:t:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'u':
				uri = strdup(optarg);
				break;

			case 'l':
				logo = strdup(optarg);
				break;

			case 's':
				pixelsize = strtod(optarg, NULL);
				break;

			case 'c':
				uc = nemocolor_parse(optarg);

				color[0] = NEMOCOLOR_DOUBLE_R(uc);
				color[1] = NEMOCOLOR_DOUBLE_G(uc);
				color[2] = NEMOCOLOR_DOUBLE_B(uc);
				break;

			case 't':
				uc = nemocolor_parse(optarg);

				textcolor[0] = NEMOCOLOR_DOUBLE_R(uc);
				textcolor[1] = NEMOCOLOR_DOUBLE_G(uc);
				textcolor[2] = NEMOCOLOR_DOUBLE_B(uc);
				break;

			default:
				break;
		}
	}

	mote = (struct moteback *)malloc(sizeof(struct moteback));
	if (mote == NULL)
		return -1;
	memset(mote, 0, sizeof(struct moteback));

	mote->width = width;
	mote->height = height;

	if (logo != NULL)
		mote->logo = logo;
	else
		mote->logo = strdup("NEMO-UX");

	mote->pixelsize = pixelsize;

	mote->colors0[0] = color[0];
	mote->colors1[0] = color[0];
	mote->colors0[1] = color[1];
	mote->colors1[1] = color[1];
	mote->colors0[2] = color[2];
	mote->colors1[2] = color[2];
	mote->colors0[3] = 0.1f;
	mote->colors1[3] = 0.3f;

	mote->tcolors0[0] = textcolor[0];
	mote->tcolors1[0] = textcolor[0];
	mote->tcolors0[1] = textcolor[1];
	mote->tcolors1[1] = textcolor[1];
	mote->tcolors0[2] = textcolor[2];
	mote->tcolors1[2] = textcolor[2];
	mote->tcolors0[3] = 1.0f;
	mote->tcolors1[3] = 1.0f;

	mote->mote = nemomote_create(3000);
	nemomote_random_set_property(&mote->random, 5.0f, 1.0f);
	nemozone_set_cube(&mote->box, mote->width * 0.0f, mote->width * 1.0f, mote->height * 0.0f, mote->height * 1.0f);
	nemozone_set_disc(&mote->disc, mote->width * 0.5f, mote->height * 0.5f, mote->width * 0.2f);
	nemozone_set_disc(&mote->speed, 0.0f, 0.0f, 50.0f);

	nemomote_blast_emit(mote->mote, 500);
	nemomote_position_update(mote->mote, &mote->box);
	nemomote_velocity_update(mote->mote, &mote->speed);
	nemomote_color_update(mote->mote,
			mote->colors1[0], mote->colors0[0],
			mote->colors1[1], mote->colors0[1],
			mote->colors1[2], mote->colors0[2],
			mote->colors1[3], mote->colors0[3]);
	nemomote_mass_update(mote->mote, 8.0f, 3.0f);
	nemomote_type_update(mote->mote, 1);
	nemomote_commit(mote->mote);

	nemomote_blast_emit(mote->mote, 50);
	nemomote_position_update(mote->mote, &mote->box);
	nemomote_velocity_update(mote->mote, &mote->speed);
	nemomote_color_update(mote->mote,
			mote->colors1[0], mote->colors0[0],
			mote->colors1[1], mote->colors0[1],
			mote->colors1[2], mote->colors0[2],
			mote->colors1[3] * 1.5f, mote->colors0[3] * 1.5f);
	nemomote_mass_update(mote->mote, 15.0f, 5.0f);
	nemomote_type_update(mote->mote, 2);
	nemomote_commit(mote->mote);

	nemoease_set(&mote->ease, NEMOEASE_CUBIC_INOUT_TYPE);

	mote->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	mote->egl = egl = nemotool_create_egl(tool);

	mote->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_input_type(NTEGL_CANVAS(canvas), NEMO_SURFACE_INPUT_TYPE_TOUCH);
	nemocanvas_set_layer(NTEGL_CANVAS(canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), moteback_dispatch_canvas_frame);

	mote->canvas = NTEGL_CANVAS(canvas);

	mote->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), moteback_dispatch_tale_event);
	nemotale_set_userdata(tale, mote);

	mote->back = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(mote->back, 0);
	nemotale_node_opaque(mote->back, 0, 0, width, height);
	nemotale_attach_node(tale, mote->back);

	moteback_render_back(mote, nemotale_node_get_pixman(mote->back), uri);

	mote->node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(mote->node, 1);
	nemotale_attach_node(tale, mote->node);

	mote->timer = nemotimer_create(tool);
	nemotimer_set_callback(mote->timer, moteback_dispatch_timer_event);
	nemotimer_set_userdata(mote->timer, mote);
	nemotimer_set_timeout(mote->timer, 5000);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	nemomote_destroy(mote->mote);

	free(mote->logo);
	free(mote);

	return 0;
}
