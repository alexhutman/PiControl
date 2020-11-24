#include <inttypes.h>
#include <xdo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_mv_mouse(xdo_t *xdo) {
	int x_0, y_0, nScreen_0;
	xdo_get_mouse_location(xdo, &x_0, &y_0, &nScreen_0);

	if (xdo_move_mouse_relative(xdo, 100, 0) != 0) {
		printf("Unable to move mouse.\n");
		return -1;
	}

	int x_f, y_f, nScreen_f;
	xdo_get_mouse_location(xdo, &x_f, &y_f, &nScreen_f);

	printf("x_0, y_0, nScreen_0: %d, %d, %d\n", x_0, y_0, nScreen_0);
	printf("x_f, y_f, nScreen_f: %d, %d, %d\n", x_f, y_f, nScreen_f);

	return 0;
}

int test_type(xdo_t *xdo) {
	//uint8_t test_char[3] = {0xD0, 0xB8, 0};
	char *test_char = "\xd0\xb8";
	//char *test_char = "H";
	//char test_char[] = {"U0438"};
	/*
	if (xdo_send_keysequence_window(xdo, CURRENTWINDOW, test_char, 12000) < 0) {
		printf("Unable to type character.\n");
		return -1;
	}
	*/
	if (xdo_enter_text_window(xdo, CURRENTWINDOW, test_char, 20000) < 0) {
		printf("Unable to type character.\n");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv) {
	/*
	while (*argv[1]) {
		printf("%02x", (unsigned int)(*argv[1]++));
	}
	printf("\n");
	return 0;
	*/


	const char *ses_type = getenv("XDG_SESSION_TYPE");
	if (strcmp(ses_type, "x11")) {
		printf("You aren't running X.\n");
		return 1;
	}

	const char *display = getenv("DISPLAY");
	xdo_t *xdo = xdo_new(display);
	if (xdo == NULL) {
		printf("Unable to create xdo_t instance\n");
		xdo_free(xdo);
		return 1;
	}

	test_mv_mouse(xdo);
	test_type(xdo);


	xdo_free(xdo);
	return 0;
}
