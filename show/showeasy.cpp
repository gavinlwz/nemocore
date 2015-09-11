#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <showeasy.h>

struct showtransition *nemoshow_transition_create_easy(struct nemoshow *show, struct showone *ease, uint32_t duration, uint32_t delay, ...)
{
	struct showtransition *trans;
	struct showone *sequence;
	va_list vargs;
	const char *name;

	trans = nemoshow_transition_create(ease, duration, delay);
	if (trans == NULL)
		return NULL;

	va_start(vargs, delay);

	while ((name = va_arg(vargs, const char *)) != NULL) {
		sequence = nemoshow_search_one(show, name);
		nemoshow_update_one_expression(show, sequence);
		nemoshow_transition_attach_sequence(trans, sequence);
	}

	va_end(vargs);

	return trans;
}

void nemoshow_attach_transition_easy(struct nemoshow *show, ...)
{
	struct showtransition *ptrans = NULL;
	struct showtransition *trans;
	va_list vargs;

	va_start(vargs, show);

	while ((trans = va_arg(vargs, struct showtransition *)) != NULL) {
		if (ptrans == NULL) {
			nemoshow_attach_transition(show, trans);
		} else {
			nemoshow_attach_transition_after(show, ptrans, trans);
		}

		ptrans = trans;
	}

	va_end(vargs);
}

struct showone *nemoshow_sequence_create_set_easy(struct nemoshow *show, struct showone *src, ...)
{
	struct showone *set;
	va_list vargs;
	const char *attr, *value;

	set = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set, src);

	va_start(vargs, src);

	while ((attr = va_arg(vargs, const char *)) != NULL && (value = va_arg(vargs, const char *)) != NULL) {
		nemoshow_sequence_set_attr(set, attr, value);
	}

	va_end(vargs);

	return set;
}

struct showone *nemoshow_sequence_create_fix_easy(struct nemoshow *show, struct showone *src, ...)
{
	struct showone *fix;
	va_list vargs;
	const char *attr, *value;

	fix = nemoshow_sequence_create_fix();
	nemoshow_sequence_fix_source(fix, src);

	va_start(vargs, src);

	while ((attr = va_arg(vargs, const char *)) != NULL && (value = va_arg(vargs, const char *)) != NULL) {
		nemoshow_sequence_fix_attr(fix, attr, value);
	}

	va_end(vargs);

	return fix;
}

struct showone *nemoshow_sequence_create_frame_easy(struct nemoshow *show, double t, ...)
{
	struct showone *frame;
	struct showone *set;
	va_list vargs;

	frame = nemoshow_sequence_create_frame();
	nemoshow_sequence_set_timing(frame, t);

	va_start(vargs, t);

	while ((set = va_arg(vargs, struct showone *)) != NULL) {
		nemoshow_one_attach_one(frame, set);
	}

	va_end(vargs);

	return frame;
}

struct showone *nemoshow_sequence_create_easy(struct nemoshow *show, ...)
{
	struct showone *sequence;
	struct showone *frame;
	va_list vargs;

	sequence = nemoshow_sequence_create();

	va_start(vargs, show);

	while ((frame = va_arg(vargs, struct showone *)) != NULL) {
		nemoshow_one_attach_one(sequence, frame);
	}

	va_end(vargs);

	return sequence;
}
