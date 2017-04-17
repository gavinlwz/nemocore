#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <timer.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoapps.h>
#include <nemolist.h>
#include <nemoitem.h>
#include <nemotoken.h>
#include <nemolog.h>
#include <nemomisc.h>

struct nemoservice *nemoenvs_attach_service(struct nemoenvs *envs, const char *type, const char *path, const char *args, const char *states)
{
	struct nemoservice *service;

	service = (struct nemoservice *)malloc(sizeof(struct nemoservice));
	if (service == NULL)
		return NULL;
	memset(service, 0, sizeof(struct nemoservice));

	service->envs = envs;

	service->type = strdup(type);
	service->path = strdup(path);
	service->args = args != NULL ? strdup(args) : NULL;
	service->states = states != NULL ? strdup(states) : NULL;

	nemolist_insert(&envs->service_list, &service->link);

	return service;
}

void nemoenvs_detach_service(struct nemoservice *service)
{
	nemolist_remove(&service->link);

	if (service->timer != NULL)
		nemotimer_destroy(service->timer);

	if (service->args != NULL)
		free(service->args);

	if (service->states != NULL)
		free(service->states);

	free(service->path);
	free(service->type);
	free(service);
}

static void nemoenvs_dispatch_service_timer(struct nemotimer *timer, void *data)
{
	struct nemoservice *service = (struct nemoservice *)data;
	struct nemoenvs *envs = service->envs;

	nemolog_warning("ENVS", "alive timeout, respawn service(%s) pid(%d)!\n", service->path, service->pid);

	kill(service->pid, SIGKILL);

	service->pid = nemoenvs_launch_app(envs, service->path, service->args, service->states);
}

int nemoenvs_alive_service(struct nemoenvs *envs, pid_t pid, uint32_t timeout)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemoservice *service, *nservice;

	nemolist_for_each_safe(service, nservice, &envs->service_list, link) {
		if (service->pid == pid) {
			if (service->timer == NULL) {
				service->timer = nemotimer_create(compz);
				nemotimer_set_callback(service->timer, nemoenvs_dispatch_service_timer);
				nemotimer_set_userdata(service->timer, service);
			}

			nemotimer_set_timeout(service->timer, timeout);

			return 1;
		}
	}

	return 0;
}

int nemoenvs_respawn_service(struct nemoenvs *envs, pid_t pid)
{
	struct nemoservice *service, *nservice;

	nemolist_for_each_safe(service, nservice, &envs->service_list, link) {
		if (service->pid == pid) {
			nemolog_warning("ENVS", "respawn service(%s) pid(%d)!\n", service->path, pid);

			service->pid = nemoenvs_launch_app(envs, service->path, service->args, service->states);

			return 1;
		}
	}

	return 0;
}

void nemoenvs_launch_services(struct nemoenvs *envs, const char *type)
{
	struct nemoservice *service;

	nemolist_for_each(service, &envs->service_list, link) {
		if (strcmp(service->type, type) == 0)
			service->pid = nemoenvs_launch_app(envs, service->path, service->args, service->states);
	}
}

int nemoenvs_attach_client(struct nemoenvs *envs, pid_t pid, const char *name)
{
	struct nemoclient *client;

	client = (struct nemoclient *)malloc(sizeof(struct nemoclient));
	if (client == NULL)
		return -1;

	client->pid = pid;
	client->name = strdup(name);
	client->stime = time_current_msecs();

	nemolist_insert(&envs->client_list, &client->link);

	nemolog_event("ENVS", "type(attach-client) name(%s) pid(%d)\n", name, pid);

	return 0;
}

int nemoenvs_detach_client(struct nemoenvs *envs, pid_t pid)
{
	struct nemoclient *client;

	nemolist_for_each(client, &envs->client_list, link) {
		if (client->pid == pid) {
			nemolist_remove(&client->link);

			nemolog_event("ENVS", "type(detach-client) name(%s) pid(%d) runtime(%u)\n", client->name, client->pid, time_current_msecs() - client->stime);

			free(client->name);
			free(client);

			return 1;
		}
	}

	return 0;
}

int nemoenvs_terminate_client(struct nemoenvs *envs, pid_t pid)
{
	struct nemoclient *client;

	nemolist_for_each(client, &envs->client_list, link) {
		if (client->pid == pid) {
			kill(client->pid, SIGKILL);

			nemolist_remove(&client->link);

			nemolog_event("ENVS", "type(terminate-client) name(%s) pid(%d) runtime(%u)\n", client->name, client->pid, time_current_msecs() - client->stime);

			free(client->name);
			free(client);

			return 1;
		}
	}

	return 0;
}

int nemoenvs_terminate_clients(struct nemoenvs *envs)
{
	struct nemoclient *client;

	nemolist_for_each(client, &envs->client_list, link) {
		kill(client->pid, SIGKILL);
	}

	return 0;
}

int nemoenvs_get_client_count(struct nemoenvs *envs)
{
	return nemolist_length(&envs->client_list);
}

int nemoenvs_launch_app(struct nemoenvs *envs, const char *_path, const char *_args, const char *_states)
{
	struct nemotoken *args;
	pid_t pid;

	args = nemotoken_create(_path, strlen(_path));
	if (_args != NULL)
		nemotoken_append_format(args, ";%s", _args);
	nemotoken_divide(args, ';');
	nemotoken_update(args);

	pid = os_execute_path(_path, nemotoken_get_tokens(args), NULL);
	if (pid > 0) {
		struct nemotoken *states;
		struct clientstate *state;

		states = nemotoken_create(_states, strlen(_states));
		nemotoken_divide(states, ';');
		nemotoken_update(states);

		state = nemoshell_create_client_state(envs->shell, pid);
		clientstate_set_attrs(state,
				nemotoken_get_tokens(states),
				nemotoken_get_count(states) / 2);

		nemotoken_destroy(states);

		nemoenvs_attach_client(envs, pid, _path);
	}

	nemotoken_destroy(args);

	return pid;
}
