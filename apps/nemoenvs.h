#ifndef	__NEMOTOOL_ENVS_H__
#define	__NEMOTOOL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>
#include <nemomonitor.h>
#include <nemomsg.h>

#include <nemobox.h>
#include <nemoitem.h>
#include <nemolist.h>

struct nemoenvs;

typedef int (*nemoenvs_callback_t)(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);

struct envscallback {
	nemoenvs_callback_t callback;
	void *data;

	struct nemolist link;
};

struct nemoenvs {
	struct nemotool *tool;

	struct nemoitem *configs;

	struct nemolist callback_list;

	struct nemomsg *msg;
	struct nemomonitor *monitor;
};

extern struct nemoenvs *nemoenvs_create(struct nemotool *tool);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_connect(struct nemoenvs *envs, const char *ip, int port);

extern int nemoenvs_set_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data);
extern int nemoenvs_put_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data);

extern int nemoenvs_dispatch(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one);

extern void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
