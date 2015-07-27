#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showone.h>
#include <showcolor.h>
#include <nemomisc.h>

void nemoshow_one_prepare(struct showone *one)
{
	nemoobject_prepare(&one->object, NEMOSHOW_ATTR_MAX);

	nemoobject_set_reserved(&one->object, "id", one->id, NEMOSHOW_ID_MAX);

	one->attrs = (struct showattr **)malloc(sizeof(struct showattr *) * 4);
	one->nattrs = 0;
	one->sattrs = 4;

	nemolist_init(&one->link);
}

void nemoshow_one_finish(struct showone *one)
{
	nemolist_remove(&one->link);

	nemoobject_finish(&one->object);

	free(one->attrs);
}

void nemoshow_one_destroy(struct showone *one)
{
	if (one->destroy != NULL) {
		one->destroy(one);
	} else {
		nemoshow_one_finish(one);

		free(one);
	}
}

struct showattr *nemoshow_one_create_attr(const char *name, const char *text, struct nemoattr *ref)
{
	struct showattr *attr;

	attr = (struct showattr *)malloc(sizeof(struct showattr));
	if (attr == NULL)
		return NULL;
	memset(attr, 0, sizeof(struct showattr));

	strcpy(attr->name, name);

	attr->text = strdup(text);
	attr->ref = ref;

	return attr;
}

void nemoshow_one_destroy_attr(struct showattr *attr)
{
	free(attr->text);
	free(attr);
}

static int nemoshow_one_compare_property(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

struct showprop *nemoshow_one_get_property(const char *name)
{
	static struct showprop props[] = {
		{ "alpha",						NEMOSHOW_DOUBLE_PROP },
		{ "begin",						NEMOSHOW_INTEGER_PROP },
		{ "cx",								NEMOSHOW_DOUBLE_PROP },
		{ "cy",								NEMOSHOW_DOUBLE_PROP },
		{ "ease",							NEMOSHOW_STRING_PROP },
		{ "end",							NEMOSHOW_INTEGER_PROP },
		{ "fill",							NEMOSHOW_COLOR_PROP },
		{ "font",							NEMOSHOW_STRING_PROP },
		{ "font-size",				NEMOSHOW_DOUBLE_PROP },
		{ "from",							NEMOSHOW_DOUBLE_PROP },
		{ "height",						NEMOSHOW_DOUBLE_PROP },
		{ "id",								NEMOSHOW_STRING_PROP },
		{ "r",								NEMOSHOW_DOUBLE_PROP },
		{ "rx",								NEMOSHOW_DOUBLE_PROP },
		{ "ry",								NEMOSHOW_DOUBLE_PROP },
		{ "src",							NEMOSHOW_STRING_PROP },
		{ "stroke",						NEMOSHOW_COLOR_PROP },
		{ "stroke-width",			NEMOSHOW_DOUBLE_PROP },
		{ "t",								NEMOSHOW_DOUBLE_PROP },
		{ "timing",						NEMOSHOW_STRING_PROP },
		{ "to",								NEMOSHOW_DOUBLE_PROP },
		{ "type",							NEMOSHOW_STRING_PROP },
		{ "width",						NEMOSHOW_DOUBLE_PROP },
		{ "x",								NEMOSHOW_DOUBLE_PROP },
		{ "x0",								NEMOSHOW_DOUBLE_PROP },
		{ "x1",								NEMOSHOW_DOUBLE_PROP },
		{ "y",								NEMOSHOW_DOUBLE_PROP },
		{ "y0",								NEMOSHOW_DOUBLE_PROP },
		{ "y1",								NEMOSHOW_DOUBLE_PROP },
	};

	return (struct showprop *)bsearch(name,
			props,
			sizeof(props) / sizeof(props[0]),
			sizeof(props[0]),
			nemoshow_one_compare_property);
}

void nemoshow_one_dump(struct showone *one, FILE *out)
{
	struct showprop *prop;
	struct showattr *attr;
	const char *name;
	int i, count;

	fprintf(out, "[%s]\n", one->id);

	count = nemoobject_get_count(&one->object);

	for (i = 0; i < count; i++) {
		name = nemoobject_get_name(&one->object, i);

		prop = nemoshow_one_get_property(name);
		if (prop != NULL) {
			if (prop->type == NEMOSHOW_DOUBLE_PROP)
				fprintf(out, "  %s = %f\n", name, nemoobject_igetd(&one->object, i));
			else if (prop->type == NEMOSHOW_INTEGER_PROP)
				fprintf(out, "  %s = %d\n", name, nemoobject_igeti(&one->object, i));
			else if (prop->type == NEMOSHOW_STRING_PROP)
				fprintf(out, "  %s = %s\n", name, nemoobject_igets(&one->object, i));
			else if (prop->type == NEMOSHOW_COLOR_PROP)
				fprintf(out, "  %s = 0x%x\n", name, nemoobject_igeti(&one->object, i));
		}
	}

	for (i = 0; i < one->nattrs; i++) {
		attr = one->attrs[i];

		fprintf(out, "  %s = %s\n", attr->name, attr->text);
	}
}
