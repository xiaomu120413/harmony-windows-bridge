#include <freerdp/freerdp.h>

#include <stdio.h>
#include <string.h>

typedef struct
{
	const char* host;
	const char* port;
	const char* user;
	const char* domain;
	const char* cert_mode;
	const char* size;
	int fullscreen;
	int clipboard;
	const char* share;
} BridgeOptions;

static const char* value_after(int argc, char** argv, int* index)
{
	if (*index + 1 >= argc)
		return NULL;
	*index += 1;
	return argv[*index];
}

static int parse_options(int argc, char** argv, BridgeOptions* options)
{
	memset(options, 0, sizeof(*options));
	options->port = "3389";
	options->cert_mode = "tofu";

	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "--host") == 0)
			options->host = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--port") == 0)
			options->port = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--user") == 0)
			options->user = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--domain") == 0)
			options->domain = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--cert-mode") == 0)
			options->cert_mode = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--size") == 0)
			options->size = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--share") == 0)
			options->share = value_after(argc, argv, &i);
		else if (strcmp(argv[i], "--fullscreen") == 0)
			options->fullscreen = 1;
		else if (strcmp(argv[i], "--clipboard") == 0)
			options->clipboard = 1;
	}

	return options->host && options->user;
}

static int print_probe(void)
{
	int major = 0;
	int minor = 0;
	int revision = 0;
	freerdp_get_version(&major, &minor, &revision);
	printf("{\"ok\":true,\"freerdpVersion\":\"%s\",\"major\":%d,\"minor\":%d,\"revision\":%d}\n",
	       freerdp_get_version_string(), major, minor, revision);
	return 0;
}

static int connect_with_library(int argc, char** argv)
{
	BridgeOptions options;
	if (!parse_options(argc, argv, &options))
	{
		fprintf(stderr, "Missing required --host or --user.\n");
		return 64;
	}

	/*
	 * This bridge already links and calls libfreerdp. The next step is to
	 * create a freerdp instance, configure rdpSettings, register update/input
	 * callbacks, and render received frames to the embedding surface.
	 */
	fprintf(stderr,
	        "FreeRDP library is available: %s\n"
	        "Library-mode connection is scaffolded but rendering/input callbacks are not implemented yet.\n"
	        "Target: %s:%s User: %s\n",
	        freerdp_get_version_string(), options.host, options.port, options.user);
	return 2;
}

static void print_usage(const char* exe)
{
	fprintf(stderr,
	        "Usage:\n"
	        "  %s --probe\n"
	        "  %s --connect --host HOST --port 3389 --user USER [options]\n",
	        exe, exe);
}

int main(int argc, char** argv)
{
	if (argc >= 2 && strcmp(argv[1], "--probe") == 0)
		return print_probe();

	if (argc >= 2 && strcmp(argv[1], "--connect") == 0)
		return connect_with_library(argc, argv);

	print_usage(argv[0]);
	return 64;
}
