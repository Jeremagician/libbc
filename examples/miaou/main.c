#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <bc.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

void usage(char *prog)
{
	fprintf(stderr, "Usage: %s self-host [group-host]\n", prog);
}

char *split(char *str, char delim)
{
	for (; *str && *str != delim; str++)
		;
	return *str == delim ? (*str = '\0', ++str) : NULL;
}

int main(int argc, char *argv[])
{
	int ret;
	char *target_host = NULL, *target_port = NULL;
	char *self_host = NULL, *self_port = NULL;
	int chan[2];
	struct bc_group *grp = alloca(bc_group_struct_size());

	if (argc < 2)
	{
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	/* TODO(Jeremy): even if this format is handy, it does not support ipv6
	 * rewrite something clever to handle it */
	if (argc == 3)
	{
		target_host = argv[2];
		target_port = split(argv[2], ':');
	}

	self_host = argv[1];
	self_port = split(argv[1], ':');

	if (argc == 3)
	{
		ret = bc_join(target_host, target_port,
					  self_host, self_port, grp, chan);
	}
	else
	{
		ret = bc_init(self_host, self_port, grp, chan);
	}

	if (ret != 0)
	{
		fprintf(stderr, "Can't init %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	for (;;)
	{
		struct bc_event ev;
		bc_poll(grp, &ev, 200);

		switch (ev.type)
		{
		case BC_JOIN:
			break;
		case BC_LEAVE:
			break;
		case BC_DEAD:
			break;
		case BC_DELIVER:
			break;

		}
	}


	bc_leave(grp);


	return EXIT_SUCCESS;
}
