#include <fcntl.h>
#include <inttypes.h>
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
	for (int i=0; i < (sizeof(valid_keys)/sizeof(int)); i++) {
		ioctl(fd, UI_SET_EVBIT, valid_keys[i]);
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

inline ssize_t emit(int fd, int type, int code, int val) {
	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	return write(fd, &ie, sizeof(ie));
}

void test_mv_mouse(int fd) {
	int i = 50;
	while (i--) {
		// Move mouse diagonally by about 7 units
		emit(fd, EV_REL, REL_X, 5);
		emit(fd, EV_REL, REL_Y, 5);
		emit(fd, EV_SYN, SYN_REPORT, 0);
		usleep(15000);
	}
}

void test_type(int fd) {
	//char *test_char = "H";
	//char *test_str = "ello world!";
	//uint8_t test_char[3] = {0xD0, 0xB8, 0};
	//char *test_char = "\xd0\xb8";
	//char test_char[] = {"U0438"};

	const int test_str[] = {KEY_H, KEY_E, KEY_L, KEY_L, KEY_O, KEY_SPACE, KEY_W, KEY_O, KEY_R, KEY_L, KEY_D};
	for (int i=0; i < (sizeof(test_str)/sizeof(int)); i++) {
		emit(fd, EV_KEY, test_str[i], 1);
		emit(fd, EV_SYN, SYN_REPORT, 0);
		emit(fd, EV_KEY, test_str[i], 0);
		emit(fd, EV_SYN, SYN_REPORT, 0);
	}
}

int main(int argc, char **argv) {
	int uinput_fd = create_uinput_fd();
	if (uinput_fd < 0) {
		fprintf(stderr, "Could not open file descriptor for new virtual device.\n");
		return 1;
	}
	sleep(1);

	test_mv_mouse(uinput_fd);
	test_type(uinput_fd);

	sleep(1);
	if (destroy_uinput_fd(uinput_fd) < 0) {
		fprintf(stderr, "Couldn't close PiControl virtual keyboard.\n");
		return 1;
	}

	return 0;
}
