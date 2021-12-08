#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


const int valid_keys[] = {
	KEY_ESC,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_Q,
	KEY_W,
	KEY_E,
	KEY_R,
	KEY_T,
	KEY_Y,
	KEY_U,
	KEY_I,
	KEY_O,
	KEY_P,
	KEY_LEFTBRACE,
	KEY_RIGHTBRACE,
	KEY_ENTER,
	KEY_LEFTCTRL,
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_GRAVE,
	KEY_LEFTSHIFT,
	KEY_BACKSLASH,
	KEY_Z,
	KEY_X,
	KEY_C,
	KEY_V,
	KEY_B,
	KEY_N,
	KEY_M,
	KEY_COMMA,
	KEY_DOT,
	KEY_SLASH,
	KEY_RIGHTSHIFT,
	KEY_LEFTALT,
	KEY_SPACE,
	KEY_CAPSLOCK,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_HOME,
	KEY_UP,
	KEY_PAGEUP,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_END,
	KEY_DOWN,
	KEY_PAGEDOWN,
	KEY_INSERT,
	KEY_DELETE,
	KEY_LEFTMETA
};

// Index = ascii char, entry = keyscan combination to produce the ascii
// Ex. ascii_to_scancodes[(unsigned int)"H" = 0x48] = [KEY_LEFTSHIFT, KEY_H]
// Each array is terminated by INT_MIN (stupid hack, probably a better way)
const int ascii_to_scancodes[][3] = {
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{KEY_BACKSPACE, INT_MIN},
	{KEY_TAB, INT_MIN},
	{KEY_ENTER, INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{KEY_ESC, INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{INT_MIN},
	{KEY_SPACE, INT_MIN},
	{KEY_LEFTSHIFT, KEY_1, INT_MIN},
	{KEY_LEFTSHIFT, KEY_APOSTROPHE, INT_MIN},
	{KEY_LEFTSHIFT, KEY_3, INT_MIN},
	{KEY_LEFTSHIFT, KEY_4, INT_MIN},
	{KEY_LEFTSHIFT, KEY_5, INT_MIN},
	{KEY_LEFTSHIFT, KEY_7, INT_MIN},
	{KEY_APOSTROPHE, INT_MIN},
	{KEY_LEFTSHIFT, KEY_9, INT_MIN},
	{KEY_LEFTSHIFT, KEY_0, INT_MIN},
	{KEY_LEFTSHIFT, KEY_8, INT_MIN},
	{KEY_LEFTSHIFT, KEY_EQUAL, INT_MIN},
	{KEY_COMMA, INT_MIN},
	{KEY_MINUS, INT_MIN},
	{KEY_DOT, INT_MIN},
	{KEY_SLASH, INT_MIN},
	{KEY_0, INT_MIN},
	{KEY_1, INT_MIN},
	{KEY_2, INT_MIN},
	{KEY_3, INT_MIN},
	{KEY_4, INT_MIN},
	{KEY_5, INT_MIN},
	{KEY_6, INT_MIN},
	{KEY_7, INT_MIN},
	{KEY_8, INT_MIN},
	{KEY_9, INT_MIN},
	{KEY_LEFTSHIFT, KEY_SEMICOLON, INT_MIN},
	{KEY_SEMICOLON, INT_MIN},
	{KEY_LEFTSHIFT, KEY_COMMA, INT_MIN},
	{KEY_EQUAL, INT_MIN},
	{KEY_LEFTSHIFT, KEY_DOT, INT_MIN},
	{KEY_LEFTSHIFT, KEY_SLASH, INT_MIN},
	{KEY_LEFTSHIFT, KEY_2, INT_MIN},
	{KEY_LEFTSHIFT, KEY_A, INT_MIN},
	{KEY_LEFTSHIFT, KEY_B, INT_MIN},
	{KEY_LEFTSHIFT, KEY_C, INT_MIN},
	{KEY_LEFTSHIFT, KEY_D, INT_MIN},
	{KEY_LEFTSHIFT, KEY_E, INT_MIN},
	{KEY_LEFTSHIFT, KEY_F, INT_MIN},
	{KEY_LEFTSHIFT, KEY_G, INT_MIN},
	{KEY_LEFTSHIFT, KEY_H, INT_MIN},
	{KEY_LEFTSHIFT, KEY_I, INT_MIN},
	{KEY_LEFTSHIFT, KEY_J, INT_MIN},
	{KEY_LEFTSHIFT, KEY_K, INT_MIN},
	{KEY_LEFTSHIFT, KEY_L, INT_MIN},
	{KEY_LEFTSHIFT, KEY_M, INT_MIN},
	{KEY_LEFTSHIFT, KEY_N, INT_MIN},
	{KEY_LEFTSHIFT, KEY_O, INT_MIN},
	{KEY_LEFTSHIFT, KEY_P, INT_MIN},
	{KEY_LEFTSHIFT, KEY_Q, INT_MIN},
	{KEY_LEFTSHIFT, KEY_R, INT_MIN},
	{KEY_LEFTSHIFT, KEY_S, INT_MIN},
	{KEY_LEFTSHIFT, KEY_T, INT_MIN},
	{KEY_LEFTSHIFT, KEY_U, INT_MIN},
	{KEY_LEFTSHIFT, KEY_V, INT_MIN},
	{KEY_LEFTSHIFT, KEY_W, INT_MIN},
	{KEY_LEFTSHIFT, KEY_X, INT_MIN},
	{KEY_LEFTSHIFT, KEY_Y, INT_MIN},
	{KEY_LEFTSHIFT, KEY_Z, INT_MIN},
	{KEY_LEFTBRACE, INT_MIN},
	{KEY_BACKSLASH, INT_MIN},
	{KEY_RIGHTBRACE, INT_MIN},
	{KEY_LEFTSHIFT, KEY_6, INT_MIN},
	{KEY_LEFTSHIFT, KEY_MINUS, INT_MIN},
	{KEY_GRAVE, INT_MIN},
	{KEY_A, INT_MIN},
	{KEY_B, INT_MIN},
	{KEY_C, INT_MIN},
	{KEY_D, INT_MIN},
	{KEY_E, INT_MIN},
	{KEY_F, INT_MIN},
	{KEY_G, INT_MIN},
	{KEY_H, INT_MIN},
	{KEY_I, INT_MIN},
	{KEY_J, INT_MIN},
	{KEY_K, INT_MIN},
	{KEY_L, INT_MIN},
	{KEY_M, INT_MIN},
	{KEY_N, INT_MIN},
	{KEY_O, INT_MIN},
	{KEY_P, INT_MIN},
	{KEY_Q, INT_MIN},
	{KEY_R, INT_MIN},
	{KEY_S, INT_MIN},
	{KEY_T, INT_MIN},
	{KEY_U, INT_MIN},
	{KEY_V, INT_MIN},
	{KEY_W, INT_MIN},
	{KEY_X, INT_MIN},
	{KEY_Y, INT_MIN},
	{KEY_Z, INT_MIN},
	{KEY_LEFTSHIFT, KEY_LEFTBRACE, INT_MIN},
	{KEY_LEFTSHIFT, KEY_BACKSLASH, INT_MIN},
	{KEY_LEFTSHIFT, KEY_RIGHTBRACE, INT_MIN},
	{KEY_LEFTSHIFT, KEY_GRAVE, INT_MIN},
	{INT_MIN}
};

int create_uinput_fd() {
	static struct uinput_setup usetup = {
		.id.bustype = BUS_USB,
		.id.vendor = 0x1337,
		.id.product = 0x0069,
		.name = "PiControl Virtual Keyboard"
	};

	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (fd < 0) {
		return fd;
	}

	// Enable device to pass key events
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	for (int i=0; i < (sizeof(valid_keys)/sizeof(valid_keys[0])); i++) {
		ioctl(fd, UI_SET_KEYBIT, valid_keys[i]);
	}

	// Enable left, right mouse button clicks, touchpad taps
	ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
	ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
	ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_DOUBLETAP);
	ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_TRIPLETAP);

	// Enable mousewheel
	ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

	// Enable mouse movement
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);

	// Set up and create device
	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	return fd;
}

int destroy_uinput_fd(int fd) {
	// TODO: Better error handling
	return (ioctl(fd, UI_DEV_DESTROY) >= 0 && close(fd) >= 0) ? 0 : -1;
}

ssize_t emit(int fd, int type, int code, int val) {
	struct input_event ie = {
		.type = type,
		.code = code,
		.value = val,
		.time.tv_sec = 0,
		.time.tv_usec = 0
	};

	return write(fd, &ie, sizeof(ie));
}

void type_char(int fd, char c) {
	// Key down
	printf("Trying to type %c  (0x%x)...\n", c, c);

	int i = 0;
	while (ascii_to_scancodes[c][i] != INT_MIN) {
		emit(fd, EV_KEY, ascii_to_scancodes[c][i++], 1);
	}
	emit(fd, EV_SYN, SYN_REPORT, 0);

	// Key up
	i = 0;
	while (ascii_to_scancodes[c][i] != INT_MIN) {
		emit(fd, EV_KEY, ascii_to_scancodes[c][i++], 0);
	}
	emit(fd, EV_SYN, SYN_REPORT, 0);
}

void test_mv_mouse(int fd) {
	int i = 50;
	while (i--) {
		// Move mouse diagonally by about 7 units
		emit(fd, EV_REL, REL_X, 5);
		emit(fd, EV_REL, REL_Y, 5);
		emit(fd, EV_SYN, SYN_REPORT, 0);
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
	emit(fd, EV_KEY, KEY_LEFTCTRL, 1);
	emit(fd, EV_KEY, KEY_C, 1);
	emit(fd, EV_SYN, SYN_REPORT, 0);
	emit(fd, EV_KEY, KEY_LEFTCTRL, 0);
	emit(fd, EV_KEY, KEY_C, 0);
	emit(fd, EV_SYN, SYN_REPORT, 0);
	*/

	const int test_str[] = {KEY_H, KEY_E, KEY_L, KEY_L, KEY_O, KEY_SPACE, KEY_W, KEY_O, KEY_R, KEY_L, KEY_D};
	for (int i=0; i < (sizeof(test_str)/sizeof(test_str[0])); i++) {
		emit(fd, EV_KEY, test_str[i], 1);
		emit(fd, EV_SYN, SYN_REPORT, 0);
		emit(fd, EV_KEY, test_str[i], 0);
		emit(fd, EV_SYN, SYN_REPORT, 0);
	}
}

void test_all_ascii_chars(int fd) {
	int j = 0;
	for (int i=0; i < sizeof(ascii_to_scancodes)/sizeof(ascii_to_scancodes[0]); i++) {
		if (ascii_to_scancodes[i][0] == INT_MIN) {
			continue;
		}
		printf("Trying to type %c  (0x%x)...\n", (char)i, i);

		type_char(fd, (char)i);

		sleep(1);
	}
}

void test_print_str(int fd, char *str) {
	char *p = str;
	while (*p) {
		type_char(fd, *p++);
	}
}

int main(int argc, char **argv) {
	int uinput_fd = create_uinput_fd();
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
	test_print_str(uinput_fd, "Hello world!");

	printf("Closing fd\n");
	sleep(1);
	if (destroy_uinput_fd(uinput_fd) < 0) {
		fprintf(stderr, "Couldn't close PiControl virtual keyboard.\n");
		return 1;
	}

	return 0;
}
