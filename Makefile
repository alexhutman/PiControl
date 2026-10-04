MAKEFLAGS      += --no-builtin-rules --no-builtin-variables
.DEFAULT_GOAL  := server

####################################### Variables ########################################

SRC_DIR        := src
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

PITEST_OBJS    := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(PITEST_C_FILES))
PER_TEST_OBJS  := $(addprefix $(OBJ_DIR)/,logging/logger.o data_structures/pool.o data_structures/queue.o)
SERVER_OBJS    := $(addsuffix .o,$(addprefix $(OBJ_DIR)/,picontrol_server networking/iputils networking/websocket_protocol serde/protocol keyboard/backend/uinput keyboard/virtual_keyboard model/protocol data_structures/pool data_structures/queue logging/logger))

ifdef USE_XDO
	SERVER_OBJS += $(OBJ_DIR)/keyboard/backend/xdo.o
endif

DEPS := $(SERVER_OBJS:.o=.d) $(PITEST_OBJS:.o=.d) $(PER_TEST_OBJS:.o=.d) $(addprefix $(OBJ_DIR)/,$(TEST_C_FILES:.c=.d))

##################################### CORE SETTINGS ######################################

CC       := gcc
CFLAGS   := -Wall -Wextra
CPPFLAGS := -I$(SRC_DIR) -MMD -MP

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

.PHONY: all server \
		create-group install-udev-rule install-binary install-service \
		uninstall pitest test check clean

# Delete target files if the command fails after it has
# started to update the file.
.DELETE_ON_ERROR:

# Never delete any intermediate files automatically.
.SECONDARY:

all: server pitest test

server: $(SERVER_TARGET)

install-udev-rule:
	install -d -m 0755 -o root -g root $(DESTDIR)/etc/udev/rules.d
	install -m 644 udev/99-picontrol-uinput.rules $(DESTDIR)/etc/udev/rules.d/99-picontrol-uinput.rules
	
	if [ -z "$(DESTDIR)" ]; then \
		udevadm control --reload-rules && udevadm trigger --verbose --sysname-match=uinput --action=change || true; \
	fi

install-binary: server
	install -d -m 0755 $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(SERVER_TARGET) $(DESTDIR)$(PREFIX)/bin/$(notdir $(SERVER_TARGET))

install-service:
	install -d -m 0700 $(DESTDIR)$(SYSD_USER_DIR)
	install -m 600 daemon/systemd/picontrol-server.service $(DESTDIR)$(SYSD_USER_DIR)/picontrol-server.service
	
	if [ -z "$(DESTDIR)" ]; then \
		systemctl --user daemon-reload; \
		systemctl --user enable picontrol-server.service; \
		systemctl --user start picontrol-server.service; \
		echo "System configuration successfully reloaded."; \
	fi

uninstall:
	if [ -z "$(DESTDIR)" ]; then \
		-systemctl --user stop picontrol-server.service; \
		-systemctl --user disable picontrol-server.service; \
		-echo "Stopped service"; \
	fi
	
	rm -f $(DESTDIR)/etc/udev/rules.d/99-picontrol-uinput.rules
	rm -f $(DESTDIR)$(SYSD_USER_DIR)/picontrol-server.service
	rm -f $(DESTDIR)$(PREFIX)/bin/$(notdir $(SERVER_TARGET))
	
	if [ -z "$(DESTDIR)" ]; then \
		udevadm control --reload-rules && udevadm trigger --verbose --sysname-match=uinput --action=change || true; \
		systemctl --user daemon-reload; \
		echo "System configuration successfully reloaded."; \
	fi

pitest: $(PITEST_TARGET)

test: $(TEST_TARGETS)
	@chmod +x $(SCRIPTS_DIR)/run_tests || true

check: test
	@$(SCRIPTS_DIR)/run_tests

clean:
	@echo "PiControl: Cleaning"
	@rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

################################### Compilation Rules ####################################

ifdef USE_XDO
$(SERVER_TARGET): LDLIBS += -lxdo
endif

$(SERVER_TARGET): LDLIBS += -lwebsockets -luv
$(SERVER_TARGET): $(SERVER_OBJS)
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

$(OBJ_DIR)/pitest/%.o: CFLAGS   += -fPIC
$(OBJ_DIR)/pitest/%.o: CPPFLAGS += -I$(TEST_DIR)
$(OBJ_DIR)/pitest/%.o: $(PITEST_SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "PiControl: Making PiTest object $@ from $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/$(TEST_DIR)/%.o: CPPFLAGS += -I$(TEST_DIR)
$(OBJ_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "PiControl: Making test object $@ from $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "PiControl: Making object $@ from $<"
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

-include $(DEPS)
