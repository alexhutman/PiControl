MAKEFLAGS      += --no-builtin-rules --no-builtin-variables
.DEFAULT_GOAL  := server

####################################### Variables ########################################

SRC_DIR        := src
INCLUDE_DIR    := include
OBJ_DIR        := obj
BIN_DIR        := bin
SCRIPTS_DIR    := scripts
LIB_DIR        := lib
TEST_DIR       := tst

PREFIX         := /usr/local
SYSD_USER_DIR  := $(HOME)/.config/systemd/user

PITEST_SRC_DIR := $(TEST_DIR)/pitest
BIN_TEST_DIR   := $(BIN_DIR)/$(TEST_DIR)

PITEST_C_FILES := $(shell find $(PITEST_SRC_DIR) -type f -name \*.c)
TEST_C_FILES   := $(shell find $(TEST_DIR) -type f -name \*_test.c)

PITEST_TARGET  := $(LIB_DIR)/libpitest.so
TEST_TARGETS   := $(addprefix $(BIN_DIR)/,$(TEST_C_FILES:.c=))
SERVER_TARGET  := $(BIN_DIR)/picontrol_server
KBD_TARGET     := $(BIN_DIR)/picontrol_daemon

PITEST_OBJS    := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(PITEST_C_FILES))
PER_TEST_OBJS  := $(addprefix $(OBJ_DIR)/shared/,logging/logger.o data_structures/pool.o data_structures/queue.o)
SERVER_OBJS    := $(addsuffix .o,$(addprefix $(OBJ_DIR)/server/,picontrol_server networking/iputils networking/websocket_protocol ipc/daemon) $(addprefix $(OBJ_DIR)/shared/,serde/protocol model/protocol data_structures/pool data_structures/queue logging/logger))
KBD_OBJS       := $(addsuffix .o,$(addprefix $(OBJ_DIR)/daemon/,keyboard_daemon keyboard/backend/uinput keyboard/virtual_keyboard) $(addprefix $(OBJ_DIR)/shared/,logging/logger data_structures/pool data_structures/queue model/protocol))

ifdef USE_XDO
	KBD_OBJS += $(OBJ_DIR)/daemon/keyboard/backend/xdo.o
endif

DEPS := $(SERVER_OBJS:.o=.d) $(KBD_OBJS:.o=.d) $(PITEST_OBJS:.o=.d) $(PER_TEST_OBJS:.o=.d) $(addprefix $(OBJ_DIR)/,$(TEST_C_FILES:.c=.d))

##################################### CORE SETTINGS ######################################

CC       := gcc
CFLAGS   := -Wall -Wextra
CPPFLAGS := -I$(INCLUDE_DIR) -MMD -MP

LDFLAGS  :=
LDLIBS   :=

ifdef DEBUG
	CPPFLAGS += -DPI_CTRL_DEBUG
	CFLAGS   += -ggdb -Og
else
	CFLAGS += -O3
endif

ifdef USE_XDO
	CPPFLAGS += -DPICTRL_XDO
endif

##################################### Phony Targets ######################################

.PHONY: all server kbd \
		create-group install-udev-rule install-binaries install-services \
		uninstall pitest test check clean

# Delete target files if the command fails after it has
# started to update the file.
.DELETE_ON_ERROR:

# Never delete any intermediate files automatically.
.SECONDARY:

all: server pitest test

server: $(SERVER_TARGET)

kbd: $(KBD_TARGET)

install-udev-rule:
	install -d -m 0755 -o root -g root $(DESTDIR)/etc/udev/rules.d
	install -m 644 udev/99-uinput.rules $(DESTDIR)/etc/udev/rules.d/99-uinput.rules
	
	@if [ -z "$(DESTDIR)" ]; then \
		udevadm control --reload-rules && udevadm trigger 2>/dev/null || true; \
	fi

install-binaries: server kbd
	install -d -m 0755 $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(SERVER_TARGET) $(DESTDIR)$(PREFIX)/bin/$(notdir $(SERVER_TARGET))
	install -m 755 $(KBD_TARGET) $(DESTDIR)$(PREFIX)/bin/$(notdir $(KBD_TARGET))
	
install-services:
	install -d -m 0700 $(DESTDIR)$(SYSD_USER_DIR)
	install -m 600 daemon/systemd/picontrol-kbd.service $(DESTDIR)$(SYSD_USER_DIR)/picontrol-kbd.service
	install -m 600 daemon/systemd/picontrol-server.service $(DESTDIR)$(SYSD_USER_DIR)/picontrol-server.service
	
	@if [ -z "$(DESTDIR)" ]; then \
		systemctl --user daemon-reload; \
		systemctl --user enable picontrol-server.service; \
		systemctl --user start picontrol-server.service; \
		systemctl --user enable picontrol-kbd.service; \
		systemctl --user start picontrol-kbd.service; \
		echo "System configurations successfully reloaded."; \
	fi

uninstall:
	@if [ -z "$(DESTDIR)" ]; then \
		-systemctl --user stop picontrol-server.service; \
		-systemctl --user disable picontrol-server.service; \
		-systemctl --user stop picontrol-kbd.service; \
		-systemctl --user disable picontrol-kbd.service; \
		-echo "Stopped services"; \
	fi
	
	rm -f $(DESTDIR)/etc/udev/rules.d/99-uinput.rules
	
	rm -f $(DESTDIR)$(SYSD_USER_DIR)/picontrol-server.service
	rm -f $(DESTDIR)$(SYSD_USER_DIR)/picontrol-kbd.service
	
	rm -f $(DESTDIR)$(PREFIX)/bin/$(notdir $(SERVER_TARGET))
	rm -f $(DESTDIR)$(PREFIX)/bin/$(notdir $(KBD_TARGET))
	
	@if [ -z "$(DESTDIR)" ]; then \
		udevadm control --reload-rules && udevadm trigger 2>/dev/null || true; \
		systemctl --user daemon-reload; \
		systemctl daemon-reload; \
		echo "System configurations successfully reloaded."; \
	fi

pitest: $(PITEST_TARGET)

test: $(TEST_TARGETS)
	@chmod +x $(SCRIPTS_DIR)/run_tests || true

check: test
	@$(SCRIPTS_DIR)/run_tests

clean:
	@echo "PiControl: Cleaning"
	@rm -rf $(OBJ_DIR) $(LIB_DIR) $(SERVER_TARGET) $(BIN_DIR)

################################### Compilation Rules ####################################

$(SERVER_TARGET): LDLIBS += -lwebsockets -luv
$(SERVER_TARGET): $(SERVER_OBJS)
	@echo "PiControl: Making $@"
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ifndef DEBUG
	@strip $@
endif

ifdef USE_XDO
$(KBD_TARGET): LDLIBS += -lxdo
endif

$(KBD_TARGET): LDLIBS += -luv
$(KBD_TARGET): $(KBD_OBJS)
	@echo "PiControl: Making $@"
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ifndef DEBUG
	@strip $@
endif

$(PITEST_TARGET): LDFLAGS += -shared
$(PITEST_TARGET): LDLIBS  += -luv
$(PITEST_TARGET): $(PITEST_OBJS)
	@mkdir -p $(dir $@)
	@echo "PiControl: Linking pitest library $@ using components: $^"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ifndef DEBUG
	@strip --strip-unneeded $@
endif

$(BIN_DIR)/$(TEST_DIR)/%_test: LDFLAGS += -L$(dir $(PITEST_TARGET))
$(BIN_DIR)/$(TEST_DIR)/%_test: LDLIBS  += -lpitest -luv
$(BIN_DIR)/$(TEST_DIR)/%_test: $(OBJ_DIR)/%.o $(OBJ_DIR)/$(TEST_DIR)/%_test.o $(PER_TEST_OBJS) | $(PITEST_TARGET)
	@mkdir -p $(dir $@)
	@echo "PiControl: Making test $@ using components: $^"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)
ifndef DEBUG
	@strip $@
endif

$(OBJ_DIR)/server/%.o: CPPFLAGS += -Isrc/server
$(OBJ_DIR)/daemon/%.o: CPPFLAGS += -Isrc/daemon

$(OBJ_DIR)/pitest/%.o: CFLAGS   += -fPIC
$(OBJ_DIR)/pitest/%.o: CPPFLAGS += -I$(TEST_DIR)
$(OBJ_DIR)/$(TEST_DIR)/%.o: CPPFLAGS += -I$(TEST_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "PiControl: Making object $@ from $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<


-include $(DEPS)
