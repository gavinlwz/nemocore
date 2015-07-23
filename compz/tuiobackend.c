#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <tuiobackend.h>
#include <tuionode.h>
#include <compz.h>
#include <nemoitem.h>
#include <nemomisc.h>

struct nemobackend *tuiobackend_create(struct nemocompz *compz)
{
	struct tuiobackend *tuio;
	struct tuionode *node;
	const char *value;
	int index = 0;
	int protocol, port, max;
	int i, count;

	tuio = (struct tuiobackend *)malloc(sizeof(struct tuiobackend));
	if (tuio == NULL)
		return NULL;
	memset(tuio, 0, sizeof(struct tuiobackend));

	tuio->base.destroy = tuiobackend_destroy;

	tuio->compz = compz;

	for (index = 0;
			(index = nemoitem_get(compz->configs, "//nemoshell/tuio", index)) >= 0;
			index++) {
		value = nemoitem_get_attr(compz->configs, index, "protocol");
		if (value == NULL || strcmp(value, "osc") != 0)
			protocol = NEMO_TUIO_XML_PROTOCOL;
		else
			protocol = NEMO_TUIO_OSC_PROTOCOL;

		value = nemoitem_get_attr(compz->configs, index, "port");
		if (value == NULL)
			port = 3333;
		else
			port = strtoul(value, 0, 10);

		value = nemoitem_get_attr(compz->configs, index, "max");
		if (value == NULL)
			max = 16;
		else
			max = strtoul(value, 0, 10);

		node = tuio_create_node(compz, protocol, port, max);
		if (node == NULL)
			break;
	}

	wl_list_insert(&compz->backend_list, &tuio->base.link);

	return &tuio->base;
}

void tuiobackend_destroy(struct nemobackend *base)
{
	struct tuiobackend *tuio = (struct tuiobackend *)container_of(base, struct tuiobackend, base);
	struct tuionode *node, *next;

	wl_list_remove(&base->link);

	wl_list_for_each_safe(node, next, &tuio->compz->tuio_list, link) {
		tuio_destroy_node(node);
	}

	free(tuio);
}
