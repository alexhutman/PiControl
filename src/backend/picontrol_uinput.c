#include "picontrol_uinput.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>

#include "logging/log_utils.h"
#include "model/protocol.h"
#include "util.h"

// `errmsg` currently MUST take exactly 1 param: the string of the error
#define IOCTL_AND_LOG_ERR(errmsg, fd, ...)       \
  {                                              \
    if (ioctl(fd, __VA_ARGS__) < 0) {            \
      pictrl_log_error(errmsg, strerror(errno)); \
    }                                            \
  }

static const pictrl_key_range valid_key_ranges[] = {
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/input-event-codes.h
    {.lower_bound = KEY_ESC, .upper_bound = KEY_KPDOT},
    {.lower_bound = KEY_F11, .upper_bound = KEY_F12}};

/*
Index ("key") = ascii char
Entry ("value") = keyscan combination to produce the ascii

Ex. pictrl_ascii_to_event_codes[(size_t)"H" = 0x48] = [KEY_LEFTSHIFT, KEY_H]
*/
static const pictrl_key_combo pictrl_ascii_to_event_codes[] = {
    // TODO: Make these repetitive ones a macro or something?
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_BACKSPACE),
    PICTRL_KEY_COMB(KEY_TAB),
    PICTRL_KEY_COMB(KEY_ENTER),

    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_ESC),

    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),
    PICTRL_NOOP_KEY_COMB(),

    PICTRL_KEY_COMB(KEY_SPACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_1),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_APOSTROPHE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_3),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_4),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_5),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_7),
    PICTRL_KEY_COMB(KEY_APOSTROPHE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_9),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_0),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_8),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_EQUAL),
    PICTRL_KEY_COMB(KEY_COMMA),
    PICTRL_KEY_COMB(KEY_MINUS),
    PICTRL_KEY_COMB(KEY_DOT),
    PICTRL_KEY_COMB(KEY_SLASH),
    PICTRL_KEY_COMB(KEY_0),
    PICTRL_KEY_COMB(KEY_1),
    PICTRL_KEY_COMB(KEY_2),
    PICTRL_KEY_COMB(KEY_3),
    PICTRL_KEY_COMB(KEY_4),
    PICTRL_KEY_COMB(KEY_5),
    PICTRL_KEY_COMB(KEY_6),
    PICTRL_KEY_COMB(KEY_7),
    PICTRL_KEY_COMB(KEY_8),
    PICTRL_KEY_COMB(KEY_9),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_SEMICOLON),
    PICTRL_KEY_COMB(KEY_SEMICOLON),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_COMMA),
    PICTRL_KEY_COMB(KEY_EQUAL),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_DOT),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_SLASH),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_2),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_A),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_B),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_C),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_D),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_E),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_F),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_G),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_H),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_I),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_J),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_K),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_L),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_M),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_N),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_O),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_P),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_Q),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_R),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_S),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_T),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_U),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_V),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_W),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_X),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_Y),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_Z),
    PICTRL_KEY_COMB(KEY_LEFTBRACE),
    PICTRL_KEY_COMB(KEY_BACKSLASH),
    PICTRL_KEY_COMB(KEY_RIGHTBRACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_6),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_MINUS),
    PICTRL_KEY_COMB(KEY_GRAVE),
    PICTRL_KEY_COMB(KEY_A),
    PICTRL_KEY_COMB(KEY_B),
    PICTRL_KEY_COMB(KEY_C),
    PICTRL_KEY_COMB(KEY_D),
    PICTRL_KEY_COMB(KEY_E),
    PICTRL_KEY_COMB(KEY_F),
    PICTRL_KEY_COMB(KEY_G),
    PICTRL_KEY_COMB(KEY_H),
    PICTRL_KEY_COMB(KEY_I),
    PICTRL_KEY_COMB(KEY_J),
    PICTRL_KEY_COMB(KEY_K),
    PICTRL_KEY_COMB(KEY_L),
    PICTRL_KEY_COMB(KEY_M),
    PICTRL_KEY_COMB(KEY_N),
    PICTRL_KEY_COMB(KEY_O),
    PICTRL_KEY_COMB(KEY_P),
    PICTRL_KEY_COMB(KEY_Q),
    PICTRL_KEY_COMB(KEY_R),
    PICTRL_KEY_COMB(KEY_S),
    PICTRL_KEY_COMB(KEY_T),
    PICTRL_KEY_COMB(KEY_U),
    PICTRL_KEY_COMB(KEY_V),
    PICTRL_KEY_COMB(KEY_W),
    PICTRL_KEY_COMB(KEY_X),
    PICTRL_KEY_COMB(KEY_Y),
    PICTRL_KEY_COMB(KEY_Z),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_LEFTBRACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_BACKSLASH),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_RIGHTBRACE),
    PICTRL_KEY_COMB(KEY_LEFTSHIFT, KEY_GRAVE),
    PICTRL_KEY_COMB(KEY_BACKSPACE)};

pictrl_uinput_t *pictrl_uinput_backend_new() {
  return malloc(sizeof(pictrl_uinput_t));
}

int pictrl_uinput_backend_init(pictrl_uinput_t *uinput) {
  int fd = picontrol_create_virtual_keyboard();
  if (fd < 0) {
    pictrl_log_error("Could not create virtual keyboard\n");
    uinput->fd = -1;
    return -1;
  }
  pictrl_log_debug("Created virtual keyboard\n");
  uinput->fd = fd;
  return 0;
}

int pictrl_uinput_backend_destroy(pictrl_uinput_t *uinput) {
  if (uinput->fd < 0) {
    pictrl_log_warn("Virtual keyboard was not open...\n");
    return -1;
  }

  int ret = picontrol_destroy_virtual_keyboard(uinput->fd);
  if (ret < 0) {
    return -1;
  }
  pictrl_log_debug("Destroyed virtual keyboard\n");
  uinput->fd = -1;
  return 0;
}

void pictrl_uinput_backend_free(pictrl_uinput_t *uinput) { free(uinput); }

void picontrol_uinput_click_mouse(pictrl_uinput_t *uinput,
                                  PiCtrlMouseBtnStatus status) {
  struct input_event ie;
  struct timeval cur_time;
  gettimeofday(&cur_time, NULL);

  int kernel_btn;
  switch (status.btn) {
    case PI_CTRL_MOUSE_LEFT:
      pictrl_log_debug("LEFT MOUSE BUTTON\n");
      kernel_btn = BTN_LEFT;
      break;
    case PI_CTRL_MOUSE_RIGHT:
      pictrl_log_debug("RIGHT MOUSE BUTTON\n");
      kernel_btn = BTN_RIGHT;
      break;
    default:
      pictrl_log_error("Invalid mouse button: %d\n", status.btn);
      return;
  }

  switch (status.click) {
    case PI_CTRL_MOUSE_DOWN:
      pictrl_log_debug("MOUSE DOWN\n");
      picontrol_emit(&ie, uinput->fd, EV_KEY, kernel_btn, PICTRL_KEY_DOWN,
                     &cur_time);
      picontrol_emit(&ie, uinput->fd, EV_SYN, SYN_REPORT, 0, &cur_time);
      break;
    case PI_CTRL_MOUSE_UP:
      pictrl_log_debug("MOUSE UP\n");
      picontrol_emit(&ie, uinput->fd, EV_KEY, kernel_btn, PICTRL_KEY_UP,
                     &cur_time);
      picontrol_emit(&ie, uinput->fd, EV_SYN, SYN_REPORT, 0, &cur_time);
      break;
    default:
      pictrl_log_error("Invalid mouse click status: %d\n", status.click);
  }
}

void picontrol_uinput_move_mouse_rel(pictrl_uinput_t *uinput,
                                     PiCtrlMouseCoord coords) {
  struct input_event ie;
  struct timeval cur_time;
  gettimeofday(&cur_time, NULL);

  picontrol_emit(&ie, uinput->fd, EV_REL, REL_X, coords.x, &cur_time);
  picontrol_emit(&ie, uinput->fd, EV_REL, REL_Y, coords.y, &cur_time);
  picontrol_emit(&ie, uinput->fd, EV_SYN, SYN_REPORT, 0, &cur_time);
}

bool picontrol_uinput_type_char(pictrl_uinput_t *uinput, char c) {
  // TODO: Error handling on `picontrol_emit` calls
  struct input_event ie;
  struct timeval cur_time;
  const size_t ie_sz = sizeof(ie);
  gettimeofday(&cur_time, NULL);
  bool ret = true;

  // Key down
  for (size_t i = 0; i < pictrl_ascii_to_event_codes[(size_t)c].num_keys; i++) {
    ret &= picontrol_emit(&ie, uinput->fd, EV_KEY,
                          pictrl_ascii_to_event_codes[(size_t)c].keys[i],
                          PICTRL_KEY_DOWN, &cur_time) == ie_sz;
    cur_time.tv_usec += PICTRL_KEY_DELAY_USEC;
  }
  ret &= picontrol_emit(&ie, uinput->fd, EV_SYN, SYN_REPORT, 0, &cur_time) ==
         ie_sz;
  if (!ret) {
    return false;
  }

  // Key up
  for (size_t i = 0; i < pictrl_ascii_to_event_codes[(size_t)c].num_keys; i++) {
    ret &= picontrol_emit(&ie, uinput->fd, EV_KEY,
                          pictrl_ascii_to_event_codes[(size_t)c].keys[i],
                          PICTRL_KEY_UP, &cur_time) == ie_sz;
    cur_time.tv_usec += PICTRL_KEY_DELAY_USEC;
  }
  ret &= picontrol_emit(&ie, uinput->fd, EV_SYN, SYN_REPORT, 0, &cur_time) ==
         ie_sz;

  return ret;
}

void picontrol_uinput_type_keysym(pictrl_uinput_t *uinput, char *keysym) {
  pictrl_log_stub("FIGURE OUT HOW TO TYPE KEYSYMS\n");
}

int picontrol_create_virtual_keyboard() {
  int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    pictrl_log_error("Could not open /dev/uinput: %s\n", strerror(errno));
    return -1;
  }

  // Enable device to pass key events
  IOCTL_AND_LOG_ERR("Could not enable key events: %s\n", fd, UI_SET_EVBIT,
                    EV_KEY);
  for (size_t i = 0; i < PICTRL_SIZE(valid_key_ranges); i++) {
    for (int key = valid_key_ranges[i].lower_bound;
         key <= valid_key_ranges[i].upper_bound; key++) {
      IOCTL_AND_LOG_ERR("Could not enable key: %s\n", fd, UI_SET_KEYBIT, key);
    }
  }

  // Enable left, right mouse button clicks, touchpad taps
  const int buttons[] = {BTN_LEFT, BTN_RIGHT, BTN_TOUCH, BTN_TOOL_DOUBLETAP,
                         BTN_TOOL_TRIPLETAP};
  for (size_t i = 0; i < PICTRL_SIZE(buttons); i++) {
    IOCTL_AND_LOG_ERR("Could not enable clicks/taps: %s\n", fd, UI_SET_KEYBIT,
                      buttons[i]);
  }

  // Enable mousewheel
  IOCTL_AND_LOG_ERR("Could not enable mousewheel: %s\n", fd, UI_SET_RELBIT,
                    REL_WHEEL);

  // Enable mouse movement
  IOCTL_AND_LOG_ERR("Could not enable mouse: %s\n", fd, UI_SET_EVBIT, EV_REL);
  IOCTL_AND_LOG_ERR("Could not enable mouse's X movement: %s\n", fd,
                    UI_SET_RELBIT, REL_X);
  IOCTL_AND_LOG_ERR("Could not enable mouse's Y movement: %s\n", fd,
                    UI_SET_RELBIT, REL_Y);

  static const struct uinput_setup usetup = {
      .id =
          {
              .bustype = BUS_USB,
              .vendor = 0x1337,
              .product = 0x0420,
          },
      .name = "PiControl Virtual Keyboard"};
  // Set up and create device
  IOCTL_AND_LOG_ERR("Could not set up virtual keyboard: %s\n", fd, UI_DEV_SETUP,
                    &usetup);
  IOCTL_AND_LOG_ERR("Could not create virtual keyboard: %s\n", fd,
                    UI_DEV_CREATE);
  return fd;
}

int picontrol_destroy_virtual_keyboard(int fd) {
  int destroy_ret = ioctl(fd, UI_DEV_DESTROY);
  if (destroy_ret < 0) {
    pictrl_log_error("Could not destroy virtual keyboard: %s\n",
                     strerror(errno));
  }

  int close_ret = close(fd);
  if (close_ret == -1) {
    pictrl_log_error("Could not close file descriptor %d: %s\n", fd,
                     strerror(errno));
  }
  return (destroy_ret >= 0 && close_ret == 0) ? 0 : -1;
}

size_t picontrol_uinput_print_str(pictrl_uinput_t *uinput, const char *str) {
  char *p = (char *)str;
  size_t chars_written = 0;
  while (*p) {
    size_t written = picontrol_uinput_type_char(uinput, *p++) ? 1 : 0;
    chars_written += written;
    if (written == 0) {
      break;
    }
  }
  return chars_written;
}
