#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include <string.h>
#include <xcb/xcb.h>

const long MS_SLEEP_MAX = (UINT_MAX + 1L) * 1000 - 1;

xcb_connection_t *conn = NULL;
char running;
char grabk = 0;
char grabp = 0;

void sig_handler(int sig)
{
	if (conn != NULL)
	{
		if (grabk)
			xcb_ungrab_keyboard(
					conn,
					XCB_CURRENT_TIME
					);
		if (grabp)
			xcb_ungrab_pointer(
					conn,
					XCB_CURRENT_TIME
					);
		xcb_flush(conn);
	}
	running = 0;
}

void check_keyboard(xcb_screen_t* screen)
{
	static xcb_grab_status_t laststatus = XCB_GRAB_STATUS_SUCCESS;

	xcb_grab_keyboard_cookie_t cookie;
	xcb_grab_keyboard_reply_t *reply;
	cookie = xcb_grab_keyboard(
			conn,
			1,                /* report events */
			screen->root,        /* grab the root window */
			XCB_CURRENT_TIME,
			XCB_GRAB_MODE_ASYNC, /* process events as normal, do not require sync */
			XCB_GRAB_MODE_ASYNC
			);
	if ((reply = xcb_grab_keyboard_reply(conn, cookie, NULL))) {
		if (reply->status != laststatus)
			switch (reply->status)
			{
				case XCB_GRAB_STATUS_SUCCESS:
					printf("grab keyboard success\n");
					break;
				case XCB_GRAB_STATUS_ALREADY_GRABBED:
					printf("grab keyboard already_grabbed\n");
					break;
				case XCB_GRAB_STATUS_FROZEN:
					printf("grab keyboard frozen\n");
					break;
				default:
					printf("grab keyboard error\n");
			}
		laststatus = reply->status;

		free(reply);
	}
	xcb_ungrab_keyboard(
			conn,
			XCB_CURRENT_TIME
			);
}

void check_pointer(xcb_screen_t* screen)
{
	static xcb_grab_status_t laststatus = XCB_GRAB_STATUS_SUCCESS;

	xcb_grab_pointer_cookie_t cookie;
	xcb_grab_pointer_reply_t *reply;
	cookie = xcb_grab_pointer(
			conn,
			1,                /* report events */
			screen->root,        /* grab the root window */
			XCB_NONE,
			XCB_GRAB_MODE_ASYNC, /* process events as normal, do not require sync */
			XCB_GRAB_MODE_ASYNC,
			XCB_NONE,
			XCB_NONE,
			XCB_CURRENT_TIME
			);
	if ((reply = xcb_grab_pointer_reply(conn, cookie, NULL))) {
		if (reply->status != laststatus)
			switch (reply->status)
			{
				case XCB_GRAB_STATUS_SUCCESS:
					printf("grab pointer success\n");
					break;
				case XCB_GRAB_STATUS_ALREADY_GRABBED:
					printf("grab pointer already_grabbed\n");
					break;
				case XCB_GRAB_STATUS_FROZEN:
					printf("grab pointer frozen\n");
					break;
				default:
					printf("grab pointer error\n");
			}
		laststatus = reply->status;

		free(reply);
	}
	xcb_ungrab_pointer(
			conn,
			XCB_CURRENT_TIME
			);
}

void sleep_ms(long ms)
{
	sleep((unsigned int)(ms / 1000));
	usleep((unsigned int)((ms % 1000) * 1000));
}

char try_parse_long(char * str, long * result)
{
	char * endptr = NULL;
	*result = strtol(str, &endptr, 10);
	if (*endptr != '\0')
		return 1;
	return 0;
}

int set_max_iters(char * str, long * max_iters_ptr)
{
	long int val = 0;
	if (try_parse_long(str, &val) != 0)
	{
		fprintf(stderr, "Invalid number: \"%s\"\n", str);
		return 4;
	}
	if (val < -1)
	{
		fprintf(stderr, "Iterations number out of range: %s\n", str);
		return 5;
	}
	*max_iters_ptr = val;
	return 0;
}

int set_delay_ms(char * str, long * delay_ms_ptr)
{
	long int val = 0;
	if (try_parse_long(str, &val) != 0)
	{
		fprintf(stderr, "Invalid number: \"%s\"\n", str);
		return 4;
	}
	if (val < 1 || val > MS_SLEEP_MAX)
	{
		fprintf(stderr, "Delay duration out of range: %s\n", str);
		return 5;
	}
	*delay_ms_ptr = val;
	return 0;
}

int main(int argc, char ** argv)
{
	long max_iters = -1;
	long delay_ms = 150;

	char show_usage = 0;

	int argi;
	for (argi = 1; argi < argc; argi++)
	{
		char * arg = argv[argi];
		if (arg[0] == '-')
		{
			if (arg[1] == '-')
			{
				arg += 2;
				if (!strcmp(arg, "pointer"))
				{
					grabp = 1;
				}
				else if (!strcmp(arg, "keyboard"))
				{
					grabk = 1;
				}
				else if (!strcmp(arg, "number"))
				{
					if (++argi >= argc)
					{
						fprintf(stderr, "Argument expected after: \"%s\"\n", arg - 2);
						return 3;
					}
					int status = set_max_iters(argv[argi], &max_iters);
					if (status)
						return status;
				}
				else if (!strcmp(arg, "delay"))
				{
					if (++argi >= argc)
					{
						fprintf(stderr, "Argument expected after: \"%s\"\n", arg - 2);
						return 3;
					}
					int status = set_delay_ms(argv[argi], &delay_ms);
					if (status)
						return status;
				}
				else if (!strcmp(arg, "help"))
				{
					show_usage = 1;
				}
				else
				{
					fprintf(stderr, "Unknown option: \"%s\"\n", arg - 2);
					return 2;
				}
			}
			else
			{
				while (arg[1] != 0)
				{
					++arg;
					switch(*arg)
					{
						case 'p': grabp = 1; break;
						case 'k': grabk = 1; break;
						case 'n': 
							if (++argi >= argc)
							{
								fprintf(stderr, "Argument expected after: \"%s\"\n", arg - 2);
								return 3;
							}
							int status = set_max_iters(argv[argi], &max_iters);
							if (status)
								return status;
							break;
						case 'd': 
							if (++argi >= argc)
							{
								fprintf(stderr, "Argument expected after: \"%s\"\n", arg - 2);
								return 3;
							}
							status = set_delay_ms(argv[argi], &delay_ms);
							if (status)
								return status;
							break;
						case 'h': show_usage = 1; break;
						default:
							fprintf(stderr, "Unknown option: \"-%c\"\n", *arg);
							return 2;
					}
				}
			}
		}
		else
		{
			fprintf(stderr, "Option expected: \"%s\"\n", arg);
			return 2;
		}
	}

	if (show_usage)
	{
		printf("Usage: %s <OPTIONS...>\n"\
				"Options:\n"\
				"  -p, --pointer           Detect pointer grabbing\n"\
				"  -k, --keyboard          Detect keyboard grabbing\n"\
				"  -n <N>, --number <N>    Iterations number, -1 for infinity\n"\
				"  -d <N>, --delay  <N>    Delay duration (ms)\n"\
				"  -h, --help              Show this message\n"\
				"Limits:\n"\
				"  -1 <= iterations number <= %li\n"\
				"   1 <= delay duration <= %li\n"\
				, argv[0], LONG_MAX, MS_SLEEP_MAX);
		return 0;
	}

	if (!grabk && !grabp)
	{
		fprintf(stderr, "--keyboard and/or --pointer should be passed\n"\
				"Pass --help for usage\n");
		return 6;
	}

	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn))
	{
		fprintf(stderr, "Error opening display.\n");
		return 1;
	}

	const xcb_setup_t* setup;
	xcb_screen_t* screen;
	setup = xcb_get_setup(conn);
	screen = xcb_setup_roots_iterator(setup).data;

	signal(SIGHUP, sig_handler);
	signal(SIGINT, sig_handler);
	signal(SIGALRM, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);

	running = 1;
	unsigned long i = 0;
	while (running)
	{
		if (max_iters != -1 && i++ >= max_iters)
			break;
		if (grabk)
			check_keyboard(screen);
		if (grabp)
			check_pointer(screen);
		xcb_flush(conn);
		if (running)
			sleep_ms(delay_ms);
	}
	return 0;
}
