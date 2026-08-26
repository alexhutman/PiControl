MAKEFLAGS      += --no-builtin-rules --no-builtin-variables
.DEFAULT_GOAL  := server

####################################### Variables ########################################

SRC_DIR        := src
OBJ_DIR        := obj
BIN_DIR        := bin
LIB_DIR        := lib
TEST_DIR       := tst

PITEST_SRC_DIR := $(TEST_DIR)/pitest
BIN_TEST_DIR   := $(BIN_DIR)/$(TEST_DIR)
INSTALL_DIR    := /usr/local/bin
SYSTEMD_DIR    ?= $(shell pkg-config systemd --variable=systemduserunitdir 2>/dev/null || echo "/usr/lib/systemd/user")

PITEST_C_FILES := $(shell find $(PITEST_SRC_DIR) -type f -name \*.c)
TEST_C_FILES   := $(shell find $(TEST_DIR) -type f -name \*_test.c)

PITEST_TARGET  := $(LIB_DIR)/libpitest.so
TEST_TARGETS   := $(addprefix $(BIN_DIR)/,$(TEST_C_FILES:.c=))
SERVER_TARGET  := $(BIN_DIR)/picontrol_server

PITEST_OBJS    := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/%.o,$(PITEST_C_FILES))
PER_TEST_OBJS  := $(addprefix $(OBJ_DIR)/,logging/logger.o data_structures/multithread_pool.o data_structures/multithread_queue.o)
SERVER_OBJS    := $(addsuffix .o,$(addprefix $(OBJ_DIR)/,picontrol_server networking/iputils networking/websocket_protocol serialize/protocol backend/picontrol_uinput backend/picontrol_backend model/protocol data_structures/multithread_pool data_structures/multithread_queue logging/logger))

ifdef USE_XDO
	SERVER_OBJS += $(OBJ_DIR)/backend/picontrol_xdo.o
endif

DEPS := $(SERVER_OBJS:.o=.d) $(PITEST_OBJS:.o=.d) $(PER_TEST_OBJS:.o=.d) $(addprefix $(OBJ_DIR)/,$(TEST_C_FILES:.c=.d))

##################################### CORE SETTINGS ######################################

CC       := gcc
CFLAGS   :=
CPPFLAGS := -I$(SRC_DIR) -MMD -MP -Wall -Wextra

LDFLAGS  :=
LDLIBS   :=

ifdef DEBUG
	CPPFLAGS += -DPI_CTRL_DEBUG
	CFLAGS   += -ggdb -Og
else
	CPPFLAGS += -O3
endif

ifdef USE_XDO
	CPPFLAGS += -DPICTRL_XDO
endif

##################################### Phony Targets ######################################

.PHONY: all server install uninstall pitest test check clean

# Delete target files if the command fails after it has
# started to update the file.
.DELETE_ON_ERROR:

# Never delete any intermediate files automatically.
.SECONDARY:

all: server pitest test

server: $(SERVER_TARGET)

install: server
	cp $(SERVER_TARGET) $(INSTALL_DIR)
	cp daemon/systemd/picontrol.service $(SYSTEMD_DIR)
	systemctl enable "$(SYSTEMD_DIR)/picontrol.service"
	systemctl start "picontrol.service"

uninstall:
	systemctl stop "picontrol.service"
	systemctl disable "picontrol.service"
	rm $(SYSTEMD_DIR)/picontrol.service
	rm $(INSTALL_DIR)/picontrol_server

pitest: $(PITEST_TARGET)

test: $(TEST_TARGETS)
	@chmod +x $(BIN_DIR)/run_tests || true

check: test
	@$(BIN_DIR)/run_tests

clean:
	@echo "PiControl: Cleaning"
	@rm -rf $(OBJ_DIR) $(LIB_DIR) $(SERVER_TARGET)
	@find $(BIN_DIR)/ -mindepth 1 -not -name "run_tests" -delete

################################### Compilation Rules ####################################

ifdef USE_XDO
$(SERVER_TARGET): LDLIBS += -lxdo
endif

$(SERVER_TARGET): LDLIBS += -lwebsockets -luv
$(SERVER_TARGET): $(SERVER_OBJS)
	@echo "PiControl: Making $@"
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

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
