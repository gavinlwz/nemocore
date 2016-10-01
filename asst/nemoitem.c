#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>

#include <nemoitem.h>
#include <nemotoken.h>

struct nemoitem *nemoitem_create(void)
{
	struct nemoitem *item;

	item = (struct nemoitem *)malloc(sizeof(struct nemoitem));
	if (item == NULL)
		return NULL;
	memset(item, 0, sizeof(struct nemoitem));

	nemolist_init(&item->list);

	return item;
}

void nemoitem_destroy(struct nemoitem *item)
{
	struct itemone *one, *next;

	nemolist_for_each_safe(one, next, &item->list, link)
		nemoitem_one_destroy(one);

	free(item);
}

void nemoitem_attach_one(struct nemoitem *item, struct itemone *one)
{
	nemolist_insert_tail(&item->list, &one->link);
}

void nemoitem_detach_one(struct nemoitem *item, struct itemone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);
}

struct itemone *nemoitem_search_one(struct nemoitem *item, const char *path)
{
	struct itemone *one;

	nemolist_for_each(one, &item->list, link) {
		if (strcmp(one->path, path) == 0)
			return one;
	}

	return NULL;
}

struct itemone *nemoitem_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value)
{
	struct itemone *one;

	nemolist_for_each(one, &item->list, link) {
		if ((path == NULL || strcmp(one->path, path) == 0) && nemoitem_one_has_attr(one, name, value) != 0)
			return one;
	}

	return NULL;
}

struct itemone *nemoitem_search_attrs(struct nemoitem *item, const char *path, char delimiter, const char *attrs)
{
	struct itemone *one;
	struct nemotoken *token;
	int ntokens;
	int i;

	token = nemotoken_create(attrs, strlen(attrs));
	nemotoken_divide(token, delimiter);
	nemotoken_update(token);

	ntokens = nemotoken_get_token_count(token) / 2;

	nemolist_for_each(one, &item->list, link) {
		if (path != NULL && strcmp(one->path, path) != 0)
			continue;

		for (i = 0; i < ntokens; i++) {
			if (nemoitem_one_has_attr(one,
						nemotoken_get_token(token, i * 2 + 0),
						nemotoken_get_token(token, i * 2 + 1)) == 0)
				break;
		}

		if (i >= ntokens) {
			nemotoken_destroy(token);

			return one;
		}
	}

	nemotoken_destroy(token);

	return NULL;
}

struct itemone *nemoitem_search_format(struct nemoitem *item, const char *path, char delimiter, const char *fmt, ...)
{
	struct itemone *one;
	va_list vargs;
	char *contents;

	va_start(vargs, fmt);
	vasprintf(&contents, fmt, vargs);
	va_end(vargs);

	one = nemoitem_search_attrs(item, path, delimiter, contents);

	free(contents);

	return one;
}

int nemoitem_count_one(struct nemoitem *item, const char *path)
{
	struct itemone *one;
	int count = 0;

	nemolist_for_each(one, &item->list, link) {
		if (strcmp(one->path, path) == 0)
			count++;
	}

	return count;
}

struct itembox *nemoitem_box_search_one(struct nemoitem *item, const char *path)
{
	struct itembox *box;
	struct itemone *one;
	int count = 0;

	nemolist_for_each(one, &item->list, link) {
		if (nemoitem_one_has_path_prefix(one, path) != 0)
			count++;
	}

	if (count == 0)
		return NULL;

	box = (struct itembox *)malloc(sizeof(struct itembox));
	if (box == NULL)
		return NULL;
	memset(box, 0, sizeof(struct itembox));

	box->ones = (struct itemone **)malloc(sizeof(struct itemone *) * count);
	if (box->ones == NULL)
		goto err1;

	nemolist_for_each(one, &item->list, link) {
		if (nemoitem_one_has_path_prefix(one, path) != 0)
			box->ones[box->nones++] = one;
	}

	return box;

err1:
	free(box);

	return NULL;
}

struct itembox *nemoitem_box_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value)
{
	struct itembox *box;
	struct itemone *one;
	int count = 0;

	nemolist_for_each(one, &item->list, link) {
		if ((path == NULL || nemoitem_one_has_path_prefix(one, path) != 0) && nemoitem_one_has_attr(one, name, value) != 0)
			count++;
	}

	if (count == 0)
		return NULL;

	box = (struct itembox *)malloc(sizeof(struct itembox));
	if (box == NULL)
		return NULL;
	memset(box, 0, sizeof(struct itembox));

	box->ones = (struct itemone **)malloc(sizeof(struct itemone *) * count);
	if (box->ones == NULL)
		goto err1;

	nemolist_for_each(one, &item->list, link) {
		if ((path == NULL || nemoitem_one_has_path_prefix(one, path) != 0) && nemoitem_one_has_attr(one, name, value) != 0)
			box->ones[box->nones++] = one;
	}

	return box;

err1:
	free(box);

	return NULL;
}

struct itembox *nemoitem_box_search_attrs(struct nemoitem *item, const char *path, char delimiter, const char *attrs)
{
	struct itembox *box;
	struct itemone *one;
	struct nemotoken *token;
	int count = 0;
	int ntokens;
	int i;

	token = nemotoken_create(attrs, strlen(attrs));
	nemotoken_divide(token, delimiter);
	nemotoken_update(token);

	ntokens = nemotoken_get_token_count(token) / 2;

	nemolist_for_each(one, &item->list, link) {
		if (path != NULL && nemoitem_one_has_path_prefix(one, path) != 0)
			continue;

		for (i = 0; i < ntokens; i++) {
			if (nemoitem_one_has_attr(one,
						nemotoken_get_token(token, i * 2 + 0),
						nemotoken_get_token(token, i * 2 + 1)) == 0)
				break;
		}

		if (i >= ntokens) {
			count++;
		}
	}

	if (count == 0)
		goto err1;

	box = (struct itembox *)malloc(sizeof(struct itembox));
	if (box == NULL)
		goto err1;
	memset(box, 0, sizeof(struct itembox));

	box->ones = (struct itemone **)malloc(sizeof(struct itemone *) * count);
	if (box->ones == NULL)
		goto err2;

	nemolist_for_each(one, &item->list, link) {
		if (path != NULL && nemoitem_one_has_path_prefix(one, path) != 0)
			continue;

		for (i = 0; i < ntokens; i++) {
			if (nemoitem_one_has_attr(one,
						nemotoken_get_token(token, i * 2 + 0),
						nemotoken_get_token(token, i * 2 + 1)) == 0)
				break;
		}

		if (i >= ntokens) {
			box->ones[box->nones++] = one;
		}
	}

	nemotoken_destroy(token);

	return box;

err2:
	free(box);

err1:
	nemotoken_destroy(token);

	return NULL;
}

struct itembox *nemoitem_box_search_format(struct nemoitem *item, const char *path, char delimiter, const char *fmt, ...)
{
	struct itembox *box;
	va_list vargs;
	char *contents;

	va_start(vargs, fmt);
	vasprintf(&contents, fmt, vargs);
	va_end(vargs);

	box = nemoitem_box_search_attrs(item, path, delimiter, contents);

	free(contents);

	return box;
}

void nemoitem_box_destroy(struct itembox *box)
{
	free(box->ones);
	free(box);
}

struct itemone *nemoitem_one_create(void)
{
	struct itemone *one;

	one = (struct itemone *)malloc(sizeof(struct itemone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct itemone));

	nemolist_init(&one->list);

	nemolist_init(&one->link);

	return one;
}

void nemoitem_one_destroy(struct itemone *one)
{
	struct itemattr *attr, *next;

	nemolist_remove(&one->link);

	if (one->path != NULL)
		free(one->path);

	nemolist_for_each_safe(attr, next, &one->list, link) {
		nemolist_remove(&attr->link);

		free(attr->name);
		free(attr->value);
		free(attr);
	}

	free(one);
}

struct itemone *nemoitem_one_clone(struct itemone *one)
{
	struct itemone *done;
	struct itemattr *attr;

	done = nemoitem_one_create();
	if (done == NULL)
		return NULL;

	nemoitem_one_set_path(done, one->path);

	nemolist_for_each(attr, &one->list, link) {
		nemoitem_one_set_attr(done, attr->name, attr->value);
	}

	return done;
}

void nemoitem_one_set_path(struct itemone *one, const char *path)
{
	if (one->path != NULL)
		free(one->path);

	one->path = strdup(path);
}

void nemoitem_one_set_path_format(struct itemone *one, const char *fmt, ...)
{
	va_list vargs;

	if (one->path != NULL)
		free(one->path);

	va_start(vargs, fmt);
	vasprintf(&one->path, fmt, vargs);
	va_end(vargs);
}

int nemoitem_one_has_path(struct itemone *one, const char *path)
{
	return strcmp(one->path, path) == 0;
}

int nemoitem_one_has_path_prefix(struct itemone *one, const char *prefix)
{
	int length = strlen(prefix);
	int i;

	for (i = 0; i < length; i++) {
		if (one->path[i] != prefix[i])
			return 0;
	}

	return 1;
}

int nemoitem_one_has_path_format(struct itemone *one, const char *fmt, ...)
{
	va_list vargs;
	char *path;
	int r;

	va_start(vargs, fmt);
	vasprintf(&path, fmt, vargs);
	va_end(vargs);

	r = strcmp(one->path, path) == 0;

	free(path);

	return r;
}

int nemoitem_one_set_attr(struct itemone *one, const char *name, const char *value)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0) {
			free(attr->value);

			attr->value = strdup(value);

			return 1;
		}
	}

	attr = (struct itemattr *)malloc(sizeof(struct itemattr));
	if (attr == NULL)
		return -1;

	attr->name = strdup(name);
	attr->value = strdup(value);

	nemolist_insert_tail(&one->list, &attr->link);

	return 0;
}

const char *nemoitem_one_get_attr(struct itemone *one, const char *name)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0)
			return attr->value;
	}

	return NULL;
}

void nemoitem_one_put_attr(struct itemone *one, const char *name)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0) {
			nemolist_remove(&attr->link);

			free(attr->name);
			free(attr->value);
			free(attr);

			break;
		}
	}
}

int nemoitem_one_has_attr(struct itemone *one, const char *name, const char *value)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0) {
			if (value == NULL || strcmp(attr->value, value) == 0)
				return 1;

			break;
		}
	}

	return 0;
}

int nemoitem_one_copy_attr(struct itemone *done, struct itemone *sone)
{
	struct itemattr *attr;

	if (sone == NULL)
		return -1;
	if (done == NULL)
		return 0;

	nemolist_for_each(attr, &sone->list, link) {
		nemoitem_one_set_attr(done, attr->name, attr->value);
	}

	return 1;
}

static int nemoitem_load_json_object(struct nemoitem *item, struct itemone *one, struct json_object *jobj)
{
	struct itemone *cone;
	struct json_object *cobj;

	if (json_object_is_type(jobj, json_type_array)) {
		int i;

		for (i = 0; i < json_object_array_length(jobj); i++) {
			cobj = json_object_array_get_idx(jobj, i);

			if (json_object_is_type(cobj, json_type_array) || json_object_is_type(cobj, json_type_object)) {
				cone = nemoitem_one_create();
				nemoitem_one_set_path(cone, one->path);
				nemoitem_attach_one(item, cone);

				nemoitem_load_json_object(item, cone, cobj);
			}
		}
	} else if (json_object_is_type(jobj, json_type_object)) {
		json_object_object_foreach(jobj, key, value) {
			if (json_object_is_type(value, json_type_array) || json_object_is_type(value, json_type_object)) {
				cone = nemoitem_one_create();
				nemoitem_one_set_path_format(cone, "%s/%s", one->path, key);
				nemoitem_attach_one(item, cone);

				nemoitem_load_json_object(item, cone, value);
			} else if (json_object_is_type(value, json_type_string)) {
				nemoitem_one_set_attr(one, key, json_object_get_string(value));
			}
		}
	}

	return 0;
}

int nemoitem_load_json(struct nemoitem *item, struct json_object *jobj)
{
	struct itemone *one;

	one = nemoitem_one_create();
	nemoitem_one_set_path(one, "/");
	nemoitem_attach_one(item, one);

	nemoitem_load_json_object(item, one, jobj);

	return 0;
}

int nemoitem_load_json_string(struct nemoitem *item, const char *contents)
{
	struct json_object *jobj;
	int r = 0;

	jobj = json_tokener_parse(contents);
	if (jobj != NULL) {
		r = nemoitem_load_json(item, jobj);

		json_object_put(jobj);
	}

	return r;
}

int nemoitem_one_load_json(struct itemone *one, struct json_object *jobj)
{
	json_object_object_foreach(jobj, key, value) {
		nemoitem_one_set_attr(one, key, json_object_get_string(value));
	}

	return 0;
}

int nemoitem_one_load_json_string(struct itemone *one, const char *contents)
{
	struct json_object *jobj;
	int r = 0;

	jobj = json_tokener_parse(contents);
	if (jobj != NULL) {
		r = nemoitem_one_load_json(one, jobj);

		json_object_put(jobj);
	}

	return r;
}
