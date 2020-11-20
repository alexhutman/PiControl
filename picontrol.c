#include <xdo.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
	const char *display = getenv("DISPLAY");
	xdo_t *xdo = xdo_new(display);
	if (xdo == NULL) {
		printf("Unable to create xdo_t instance\n");
		xdo_free(xdo);
		return 1;
	}

	int x_0, y_0, nScreen_0;
	xdo_get_mouse_location(xdo, &x_0, &y_0, &nScreen_0);

	if (xdo_move_mouse_relative(xdo, 100, 0) != 0) {
		printf("Unable to move mouse.\n");
		return 2;
	}

	int x_f, y_f, nScreen_f;
	xdo_get_mouse_location(xdo, &x_f, &y_f, &nScreen_f);

	printf("x_0, y_0, nScreen_0: %d, %d, %d\n", x_0, y_0, nScreen_0);
	printf("x_f, y_f, nScreen_f: %d, %d, %d\n", x_f, y_f, nScreen_f);

	xdo_free(xdo);
	return 0;
}
