#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <bc.h>

#include <string.h>
#include <errno.h>

int main(void)
{
	int ret;
	struct bc_group *grp = alloca(bc_group_struct_size());
	int chan[2];

	ret = bc_init(NULL, NULL, grp, chan);
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
