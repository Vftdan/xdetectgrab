#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <xcb/xcb.h>

xcb_connection_t *conn = NULL;
char running;
char grabk = 1;
char grabp = 1;

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

int main(int argc, char ** argv)
{
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
	while (running)
	{
		if (grabk)
			check_keyboard(screen);
		if (grabp)
			check_pointer(screen);
		xcb_flush(conn);
		usleep(150000);
	}
	return 0;
}
