#include <inttypes.h>
#include <limits.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "picontrol_uinput.h"


void test_mv_mouse(int fd) {
	int i = 50;
	while (i--) {
		// Move mouse diagonally by about 7 units
		picontrol_emit(fd, EV_REL, REL_X, 5);
		picontrol_emit(fd, EV_REL, REL_Y, 5);
		picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);
	}
}

void test_type(int fd) {
	//char *test_char = "H";
	//char *test_str = "ello world!";
	//uint8_t test_char[3] = {0xD0, 0xB8, 0};
	//char *test_char = "\xd0\xb8";
	//char test_char[] = {"U0438"};

	/*
	// Ctrl+C
	picontrol_emit(fd, EV_KEY, KEY_LEFTCTRL, 1);
	picontrol_emit(fd, EV_KEY, KEY_C, 1);
	picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);
	picontrol_emit(fd, EV_KEY, KEY_LEFTCTRL, 0);
	picontrol_emit(fd, EV_KEY, KEY_C, 0);
	picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);
	*/

	const int test_str[] = {KEY_H, KEY_E, KEY_L, KEY_L, KEY_O, KEY_SPACE, KEY_W, KEY_O, KEY_R, KEY_L, KEY_D};
	for (int i=0; i < (sizeof(test_str)/sizeof(test_str[0])); i++) {
		picontrol_emit(fd, EV_KEY, test_str[i], 1);
		picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);
		picontrol_emit(fd, EV_KEY, test_str[i], 0);
		picontrol_emit(fd, EV_SYN, SYN_REPORT, 0);
	}
}

void test_all_ascii_chars(int fd) {
	int j = 0;
	for (int i=0; i < len_ascii_to_scancodes; i++) {
		if (ascii_to_scancodes[i][0] == INT_MIN) {
			continue;
		}
		printf("Trying to type %c  (0x%x)...\n", (char)i, i);

		picontrol_type_char(fd, (char)i);

		sleep(1);
	}
}

int main(int argc, char **argv) {
	int uinput_fd = picontrol_create_uinput_fd();
	if (uinput_fd < 0) {
		fprintf(stderr, "Could not open file descriptor for new virtual device.\n");
		return 1;
	}
	sleep(1);
	printf("Created virtual keyboard\n");

	test_mv_mouse(uinput_fd);
	printf("Typing...\n");
	test_type(uinput_fd);
	test_all_ascii_chars(uinput_fd);
	picontrol_print_str(uinput_fd, "echo Hello World!\n");

	printf("Closing fd\n");
	sleep(1);
	if (picontrol_destroy_uinput_fd(uinput_fd) < 0) {
		fprintf(stderr, "Couldn't close PiControl virtual keyboard.\n");
		return 1;
	}

	return 0;
}
