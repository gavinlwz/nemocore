#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <json.h>

#include <nemojson.h>
#include <nemomisc.h>

struct nemojson *nemojson_create(const char *str, int length)
{
	struct nemojson *json;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	json->contents = (char *)malloc(length + 1);
	if (json->contents == NULL)
		goto err1;
	memcpy(json->contents, str, length);

	json->length = length;

	return json;

err1:
	free(json);

	return NULL;
}

struct nemojson *nemojson_create_format(const char *fmt, ...)
{
	struct nemojson *json;
	va_list vargs;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	va_start(vargs, fmt);
	vasprintf(&json->contents, fmt, vargs);
	va_end(vargs);

	json->length = strlen(json->contents);

	return json;
}

struct nemojson *nemojson_create_file(const char *filepath)
{
	struct nemojson *json;
	char *buffer = NULL;
	int length;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	length = os_load_path(filepath, &buffer);
	if (length <= 0)
		goto err1;

	json->contents = buffer;
	json->length = length;

	return json;

err1:
	free(json);

	return NULL;
}

void nemojson_destroy(struct nemojson *json)
{
	int i;

	for (i = 0; i < json->count; i++)
		json_object_put(json->jobjs[i]);

	free(json->contents);
	free(json);
}

int nemojson_append(struct nemojson *json, const char *str, int length)
{
	char *contents;

	contents = (char *)malloc(json->length + length + 1);
	if (contents == NULL)
		return -1;

	strcpy(contents, json->contents);
	strcat(contents, str);
	contents[json->length + length] = '\0';

	free(json->contents);

	json->contents = contents;
	json->length = json->length + length;

	return 0;
}

int nemojson_append_one(struct nemojson *json, char c)
{
	char *contents;

	contents = (char *)malloc(json->length + 1 + 1);
	if (contents == NULL)
		return -1;

	snprintf(contents, json->length + 1 + 1, "%s%c", json->contents, c);

	free(json->contents);

	json->contents = contents;
	json->length = json->length + 1;

	return 0;
}

int nemojson_append_format(struct nemojson *json, const char *fmt, ...)
{
	va_list vargs;
	char *contents;
	char *str;
	int length;

	va_start(vargs, fmt);
	vasprintf(&str, fmt, vargs);
	va_end(vargs);

	length = strlen(str);

	contents = (char *)malloc(json->length + length + 1);
	if (contents == NULL)
		return -1;

	strcpy(contents, json->contents);
	strcat(contents, str);
	contents[json->length + length] = '\0';

	free(json->contents);
	free(str);

	json->contents = contents;
	json->length = json->length + length;

	return 0;
}

void nemojson_update(struct nemojson *json)
{
	struct json_tokener *jtok;
	struct json_object *jobj;
	char *msg;

	jtok = json_tokener_new();

	for (msg = json->contents; msg < json->contents + json->length; msg += jtok->char_offset) {
		jobj = json_tokener_parse_ex(jtok, msg, strlen(msg));
		if (jobj == NULL)
			break;

		json->jobjs[json->count++] = jobj;
	}

	json_tokener_free(jtok);
}

static inline struct json_object *nemojson_search_object_vargs(struct nemojson *json, int index, int depth, va_list vargs)
{
	struct json_object *jobj;
	const char *key;
	int i;

	jobj = json->jobjs[index];
	if (jobj == NULL)
		return NULL;

	for (i = 0; i < depth; i++) {
		key = va_arg(vargs, const char *);
		if ('0' <= key[0] && key[0] <= '9') {
			if (json_object_is_type(jobj, json_type_array) == 0)
				goto nofound;
			if ((jobj = json_object_array_get_idx(jobj, strtoul(key, NULL, 10))) == NULL)
				goto nofound;
		} else {
			if (json_object_is_type(jobj, json_type_object) == 0)
				goto nofound;
			if (json_object_object_get_ex(jobj, key, &jobj) == 0)
				goto nofound;
		}
	}

	return jobj;

nofound:
	return NULL;
}

struct json_object *nemojson_search_object(struct nemojson *json, int index, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	return jobj;
}

int nemojson_search_integer(struct nemojson *json, int index, int value, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	if (jobj == NULL)
		return value;

	return json_object_get_int(jobj);
}

double nemojson_search_double(struct nemojson *json, int index, double value, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	if (jobj == NULL)
		return value;

	return json_object_get_double(jobj);
}

const char *nemojson_search_string(struct nemojson *json, int index, const char *value, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	if (jobj == NULL)
		return value;

	return json_object_get_string(jobj);
}

struct json_object *nemojson_search_attribute(struct json_object *jobj, const char *key, const char *value)
{
	if (json_object_is_type(jobj, json_type_object) != 0) {
		struct json_object_iterator citer = json_object_iter_begin(jobj);
		struct json_object_iterator eiter = json_object_iter_end(jobj);
		struct json_object *tobj;

		while (json_object_iter_equal(&citer, &eiter) == 0) {
			const char *ikey = json_object_iter_peek_name(&citer);
			struct json_object *iobj = json_object_iter_peek_value(&citer);

			if (json_object_is_type(iobj, json_type_string) != 0 &&
					strcmp(ikey, key) == 0 &&
					strcmp(json_object_get_string(iobj), value) == 0)
				return jobj;

			tobj = nemojson_search_attribute(iobj, key, value);
			if (tobj != NULL)
				return tobj;
		}
	} else if (json_object_is_type(jobj, json_type_object) != 0) {
		struct json_object *tobj;
		int i;

		for (i = 0; i < json_object_array_length(jobj); i++) {
			tobj = nemojson_search_attribute(json_object_array_get_idx(jobj, i), key, value);
			if (tobj != NULL)
				return tobj;
		}
	}

	return NULL;
}

struct json_object *nemojson_object_get_object(struct json_object *jobj, const char *name, struct json_object *value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return tobj;
}

const char *nemojson_object_get_string(struct json_object *jobj, const char *name, const char *value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_string(tobj);
}

double nemojson_object_get_double(struct json_object *jobj, const char *name, double value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_double(tobj);
}

int nemojson_object_get_integer(struct json_object *jobj, const char *name, int value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_int(tobj);
}

int nemojson_array_get_length(struct json_object *jobj)
{
	return json_object_array_length(jobj);
}

struct json_object *nemojson_array_get_object(struct json_object *jobj, int index)
{
	return json_object_array_get_idx(jobj, index);
}
