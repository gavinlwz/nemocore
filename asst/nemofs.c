#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <dirent.h>
#include <regex.h>

#include <nemofs.h>
#include <oshelper.h>
#include <nemomisc.h>

struct fsdir *nemofs_dir_create(const char *path, int minimum_files)
{
	struct fsdir *dir;
	struct dirent **entries;
	int count = 0;

	dir = (struct fsdir *)malloc(sizeof(struct fsdir));
	if (dir == NULL)
		return NULL;
	memset(dir, 0, sizeof(struct fsdir));

	if (path != NULL) {
		count = scandir(path, &entries, NULL, alphasort);
		if (count > 0)
			free(entries);
	}

	count = MAX(count, minimum_files);

	dir->filenames = (char **)malloc(sizeof(char *) * count);
	dir->filepaths = (char **)malloc(sizeof(char *) * count);
	dir->nfiles = 0;
	dir->mfiles = count;
	dir->path = path != NULL ? strdup(path) : NULL;

	return dir;
}

void nemofs_dir_destroy(struct fsdir *dir)
{
	int i;

	for (i = 0; i < dir->nfiles; i++) {
		free(dir->filenames[i]);
		free(dir->filepaths[i]);
	}

	free(dir->filenames);
	free(dir->filepaths);

	if (dir->path != NULL)
		free(dir->path);

	free(dir);
}

void nemofs_dir_clear(struct fsdir *dir)
{
	int i;

	for (i = 0; i < dir->nfiles; i++) {
		free(dir->filenames[i]);
		free(dir->filepaths[i]);
	}

	dir->nfiles = 0;
}

int nemofs_dir_scan_directories(struct fsdir *dir)
{
	struct dirent **entries;
	const char *filename;
	char filepath[128];
	int nfiles = 0;
	int i, count;

	count = scandir(dir->path, &entries, NULL, alphasort);
	count = MIN(count, dir->mfiles - dir->nfiles);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, dir->path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_directory(filepath) != 0) {
			dir->filenames[dir->nfiles + nfiles] = strdup(filename);
			dir->filepaths[dir->nfiles + nfiles] = strdup(filepath);
			nfiles++;
		}
	}

	dir->nfiles += nfiles;

	free(entries);

	return nfiles;
}

int nemofs_dir_scan_files(struct fsdir *dir)
{
	struct dirent **entries;
	const char *filename;
	char filepath[128];
	int nfiles = 0;
	int i, count;

	count = scandir(dir->path, &entries, NULL, alphasort);
	count = MIN(count, dir->mfiles - dir->nfiles);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, dir->path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_file(filepath) != 0) {
			dir->filenames[dir->nfiles + nfiles] = strdup(filename);
			dir->filepaths[dir->nfiles + nfiles] = strdup(filepath);
			nfiles++;
		}
	}

	dir->nfiles += nfiles;

	free(entries);

	return nfiles;
}

int nemofs_dir_scan_extension(struct fsdir *dir, const char *extension)
{
	struct dirent **entries;
	const char *filename;
	char filepath[128];
	int nfiles = 0;
	int i, count;

	count = scandir(dir->path, &entries, NULL, alphasort);
	count = MIN(count, dir->mfiles - dir->nfiles);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, dir->path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_file(filepath) != 0 && os_has_file_extension(filename, extension) != 0) {
			dir->filenames[dir->nfiles + nfiles] = strdup(filename);
			dir->filepaths[dir->nfiles + nfiles] = strdup(filepath);
			nfiles++;
		}
	}

	dir->nfiles += nfiles;

	free(entries);

	return nfiles;
}

int nemofs_dir_scan_regex(struct fsdir *dir, const char *expr)
{
	struct dirent **entries;
	regex_t regex;
	const char *filename;
	char filepath[128];
	int nfiles = 0;
	int i, count;

	if (regcomp(&regex, expr, REG_EXTENDED))
		return 0;

	count = scandir(dir->path, &entries, NULL, alphasort);
	count = MIN(count, dir->mfiles - dir->nfiles);

	for (i = 0; i < count; i++) {
		filename = entries[i]->d_name;

		strcpy(filepath, dir->path);
		strcat(filepath, "/");
		strcat(filepath, filename);

		if (os_check_path_is_file(filepath) != 0 && regexec(&regex, filename, 0, NULL, 0) == 0) {
			dir->filenames[dir->nfiles + nfiles] = strdup(filename);
			dir->filepaths[dir->nfiles + nfiles] = strdup(filepath);
			nfiles++;
		}
	}

	dir->nfiles += nfiles;

	free(entries);

	regfree(&regex);

	return nfiles;
}

int nemofs_dir_insert_file(struct fsdir *dir, const char *filename)
{
	if (dir->nfiles >= dir->mfiles)
		return -1;

	dir->filenames[dir->nfiles] = strdup(filename);

	if (dir->path == NULL)
		dir->filepaths[dir->nfiles] = strdup(filename);
	else
		asprintf(&dir->filepaths[dir->nfiles], "%s/%s", dir->path, filename);

	return dir->nfiles++;
}