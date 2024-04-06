MAKEFLAGS += --no-builtin-rules --no-builtin-variables

################################## Variables ###################################
BASE_DIR       := .

#All relative
SRC_DIR        := src
BIN_DIR        := bin
TEST_DIR       := tst
BIN_TEST_DIR   := $(BIN_DIR)/$(TEST_DIR)

SERVER         := $(BIN_DIR)/picontrol_server
TEST_SCRIPT    := $(BIN_DIR)/run_tests
PITEST_SO_PATH := $(BIN_DIR)/pitest/pitest.so

SERVER_OBJS    := $(SRC_DIR)/picontrol_server.o $(SRC_DIR)/networking/iputils.o $(SRC_DIR)/data_structures/ring_buffer.o $(SRC_DIR)/backend/picontrol_uinput.o $(SRC_DIR)/backend/picontrol_backend.o

PITEST_SRC_DIR := $(TEST_DIR)/pitest
PITEST_C_FILES := $(shell find $(PITEST_SRC_DIR) -type f -name \*.c)
PITEST_OBJ     := $(PITEST_C_FILES:.c=.o)

TEST_FILES     := $(shell find $(TEST_DIR) -type f -name \*_test.c)
TEST_TARGETS   := $(addprefix $(BIN_DIR)/,$(TEST_FILES:.c=))

# Full paths
SRC_DIR_FULL   := $(BASE_DIR)/$(SRC_DIR)
TEST_DIR_FULL  := $(BASE_DIR)/$(TEST_DIR)

CC             := gcc
CFLAGS         := -MMD -MP -Wall -Wextra

ifdef DEBUG
	CFLAGS += -DPI_CTRL_DEBUG -ggdb -Og
else
	CFLAGS += -O3
endif

XDO_FLAG :=
ifdef USE_XDO
	CFLAGS      += -DPICTRL_XDO
	SERVER_OBJS += $(SRC_DIR)/backend/pictrl_xdo.o
	XDO_FLAG    += -lxdo
endif

################################ Phony Targets #################################
.PHONY: all picontrol server pitest test clean
all: picontrol server pitest test

server: $(SERVER)

pitest: $(PITEST_SO_PATH)

test: $(TEST_TARGETS) | $(TEST_SCRIPT)

clean:
	$(info PiControl: Cleaning)
	find $(BIN_DIR)/ -mindepth 1 | grep -v "$(TEST_SCRIPT)" | xargs -r rm -rf
	find $(SRC_DIR)/ $(TEST_DIR)/ -type f \( -name \*.o -o -name \*.d \) | xargs -r rm

################################### Targets ####################################
$(SERVER): $(SERVER_OBJS)
	$(info PiControl: Making $@)
	$(CC) $^ -o $@ $(XDO_FLAG) -I$(SRC_DIR_FULL)

$(PITEST_SO_PATH): $(PITEST_OBJ)
	$(info PiControl: Linking pitest library $@ using components: $^)
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) -shared -o $@ $^
ifndef DEBUG
	strip "$@"
endif

$(PITEST_SRC_DIR)/%.o: $(PITEST_SRC_DIR)/%.c $(PITEST_SRC_DIR)/%.h
	$(info PiControl: Compiling pitest library component $@)
	$(CC) $(CFLAGS) -fPIC -o $@ -c $< -I$(SRC_DIR_FULL) -I$(TEST_DIR_FULL)

################################################################################

$(BIN_TEST_DIR)/%_test: $(SRC_DIR)/%.o $(TEST_DIR)/%_test.o | $(PITEST_SO_PATH)
	$(info PiControl: Creating test executable $@)
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) $^ -o $@ -L$(dir $|) -l:$(notdir $|)
ifndef DEBUG
	strip "$@"
endif

$(TEST_DIR)/%_test.o: $(TEST_DIR)/%_test.c | $(SRC_DIR)/%.o
	$(info PiControl: Compiling test object $@)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(SRC_DIR_FULL) -I$(TEST_DIR_FULL)

# If the prereq has an associated header, recompile obj when header changes
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(info PiControl: Compiling source object $@)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(SRC_DIR_FULL)

# Otherwise, just compile C file when it changes
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(info PiControl: Compiling source object $@)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(SRC_DIR_FULL)

$(TEST_SCRIPT)::
	@[ -f "$@" ] && [ ! -x "$@" ] && chmod +x "$@" || true
