#ifndef _PICTRL_UINPUT_H
#define _PICTRL_UINPUT_H


extern const int valid_keyboard_keys[];
extern const int len_valid_keyboard_keys;

extern const int ascii_to_scancodes[][3];
extern const int len_ascii_to_scancodes;

int picontrol_create_uinput_fd();
int picontrol_destroy_uinput_fd(int fd);
static ssize_t picontrol_emit(int fd, int type, int code, int val);
static void picontrol_type_char(int fd, char c);
void picontrol_print_str(int fd, char *str);

#endif
