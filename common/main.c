#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gl_backend.h"
#include "vk_backend.h"
#include "utils.h"


#define HELP_MESSAGE \
    "Usage:\tmainCraft.run --backend vulkan\n" \
    "Options:\n" \
    "\t-b,\t--backend\t Selct backend (vulkan or opengl).\n" \
    "\t-h,\t--help\t Show This Message.\n\n" \


enum backend_type { vulkan, opengl };

// Inicialization of long options of opt
#define LONG_OPTIONS \
	{ \
		{"backend", required_argument, NULL, 'b'}, \
		{"help", no_argument, NULL, 'h'}, \
		{0, 0, 0, 0} \
	}

/* main program */
int
main(const int argc, char *const *argv)
{
	enum backend_type backend = vulkan;
	struct option longOptions[] = LONG_OPTIONS;
	int option = 0;

	while ((option = getopt_long(argc, argv, "b:h", longOptions, NULL)) != -1) {
		switch (option){
		case 'b':
			if (!strcmp(optarg, "vulkan"))
				backend = vulkan;
			else if (!strcmp(optarg, "opengl"))
				backend = opengl;
			else {
				pprint_error("'%s' is no a valid backend\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'h':
			printf(HELP_MESSAGE);
			exit(EXIT_SUCCESS);
		case '?':
			// flush opt message
			fflush(stderr);
			printf("Try --help\n");
			exit(EXIT_FAILURE);
		}
	}

	if (backend == opengl)
		exit(run_gl(argc, argv));
	else
		exit(run_vk(argc, argv));
}

