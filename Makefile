# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

################################## Variables ###################################
BASE_DIR       := .

#All relative
SRC_DIR        := src
BIN_DIR        := bin
TEST_DIR       := tst
BIN_TEST_DIR   := $(BIN_DIR)/$(TEST_DIR)

EXE            := $(BIN_DIR)/picontrol
SERVER         := $(BIN_DIR)/picontrol_server
TEST_SCRIPT    := $(BIN_DIR)/run_tests
PITEST_SO_PATH := $(BIN_DIR)/pitest/pitest.so

PITEST_SRC_DIR := $(TEST_DIR)/pitest
PITEST_C_FILES := $(shell find $(PITEST_SRC_DIR) -type f -name \*.c)
PITEST_OBJ     := $(PITEST_C_FILES:.c=.o)

TEST_FILES     := $(shell find $(TEST_DIR) -type f -name \*_test.c)
TEST_TARGETS   := $(addprefix $(BIN_DIR)/,$(TEST_FILES:.c=))

# Full paths
SRC_DIR_FULL   := $(BASE_DIR)/$(SRC_DIR)
TEST_DIR_FULL  := $(BASE_DIR)/$(TEST_DIR)

CC             := gcc
CFLAGS         := -MMD -MP

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG -ggdb -Og
else
	override CFLAGS += -O3
endif

################################ Phony Targets #################################
.PHONY: all picontrol server pitest test clean
all: picontrol server pitest test

picontrol: $(EXE)

server: $(SERVER)

pitest: $(PITEST_SO_PATH)

test: $(TEST_TARGETS) | $(TEST_SCRIPT)

clean:
	$(info PiControl: Cleaning)
	find $(BIN_DIR)/ -mindepth 1 | grep -v "$(TEST_SCRIPT)" | xargs -r rm -rf
	find $(SRC_DIR)/ $(TEST_DIR)/ -type f \( -name \*.o -o -name \*.d \) | xargs -r rm

################################### Targets ####################################
$(EXE): $(SRC_DIR)/picontrol.o $(SRC_DIR)/picontrol_uinput.o
	$(info PiControl: Making $@)
	$(CC) $^ -o $@ -I$(SRC_DIR_FULL)

$(SERVER): $(SRC_DIR)/picontrol_server.o $(SRC_DIR)/picontrol_iputils.o
	$(info PiControl: Making $@)
	$(CC) $^ -o $@ -lxdo -I$(SRC_DIR_FULL)

$(PITEST_SO_PATH): $(PITEST_OBJ)
	$(info PiControl: Linking pitest library $@ using components: $^)
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) -shared -o $@ $^

$(PITEST_SRC_DIR)/%.o: $(PITEST_SRC_DIR)/%.c
	$(info PiControl: Compiling pitest library component $@)
	$(CC) $(CFLAGS) -I$(TEST_DIR_FULL) -I$(SRC_DIR_FULL) -c -fPIC $^ -o $@

################################################################################

$(BIN_TEST_DIR)/%_test: $(TEST_DIR)/%_test.a | $(PITEST_SO_PATH)
	$(info PiControl: Creating test executable $@)
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) $< -o $@ -L$(dir $|) -l:$(notdir $|)

$(TEST_DIR)/%_test.a: $(SRC_DIR)/%.o $(TEST_DIR)/%_test.o
	$(info PiControl: Stitching together test lib $@ [only $? changed])
	ar -rcs $@ $?

$(TEST_DIR)/%_test.o: $(TEST_DIR)/%_test.c | $(SRC_DIR)/%.o
	$(info PiControl: Compiling test object $@)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(SRC_DIR_FULL) -I$(TEST_DIR_FULL)
	touch $|

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/%.h
	$(info PiControl: Compiling source object $@)
	$(CC) $(CFLAGS) -o $@ -c $< -I$(SRC_DIR_FULL)

$(TEST_DIR)/%_test.c:: $(SRC_DIR)/%.c

$(TEST_SCRIPT)::
	@[ -f "$@" ] && [ ! -x "$@" ] && chmod +x "$@" || true
