################################## Variables ###################################
SRC_DIR    := ./src
BIN_DIR    := ./bin
TEST_DIR   := ./tst
BIN_TEST_DIR    := $(patsubst ./%,$(BIN_DIR)/%,$(TEST_DIR))

EXE        := $(BIN_DIR)/picontrol
SERVER     := $(BIN_DIR)/picontrol_server
TEST_SCRIPT := $(BIN_DIR)/run_tests.sh

TEST_UTILS_MIDDLE := utils/pictrl_test_utils.so
TEST_UTILS_LIB := $(BIN_DIR)/$(TEST_UTILS_MIDDLE)
TEST_UTILS_DIR := $(dir $(TEST_UTILS_LIB))
TEST_UTILS_OBJ := $(TEST_DIR)/$(TEST_UTILS_MIDDLE:.so=.o)
TEST_UTILS_C := $(TEST_UTILS_OBJ:.o=.c)
RING_BUFFER := $(BIN_DIR)/data_structures/ring_buffer.so
TEST_FILES := $(shell find $(TEST_DIR) -type f -name \*_test.c)
TEST_TARGETS := $(patsubst ./%,$(BIN_DIR)/%,$(TEST_FILES:.c=))
LD_ENV_VAR := LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(TEST_UTILS_DIR)

CC         := gcc
CFLAGS     := -O3 -MMD -MP
LDFLAGS    :=
LDLIBS     :=
LDFLAGS    := $(if $(strip $(LDFLAGS)),-l $(LDFLAGS),)
LDLIBS     := $(if $(strip $(LDLIBS)),-l $(LDLIBS),)

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG
endif


################################ Phony Targets #################################
.PHONY: all picontrol server test clean
all: picontrol server test

picontrol: $(EXE)

server: $(SERVER)

test: $(TEST_TARGETS) $(TEST_SCRIPT)
	chmod +x $(TEST_SCRIPT)

clean:
	find $(BIN_DIR)/ -mindepth 1 | grep -v "$(TEST_SCRIPT)" | xargs -r rm -rf
	find $(SRC_DIR)/ $(TEST_DIR)/ -type f \( -name \*.o -o -name \*.d \) | xargs -r rm


################################### Targets ####################################
$(EXE): $(SRC_DIR)/picontrol.o $(SRC_DIR)/picontrol_uinput.o | $(BIN_DIR)
	@echo "PiControl: Making $@"
	$(CC) $^ -o $@ $(LDFLAGS) -I$(SRC_DIR)

$(SERVER): $(SRC_DIR)/picontrol_server.o $(SRC_DIR)/picontrol_iputils.o | $(BIN_DIR)
	@echo "PiControl: Making $@"
	$(CC) $^ -o $@ -lxdo $(LDFLAGS) -I$(SRC_DIR)

$(TEST_UTILS_LIB): $(TEST_UTILS_OBJ)
	@echo "PiControl: Linking test utils lib $@ using $^"
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) -shared -o $@ $^

$(TEST_UTILS_OBJ): $(TEST_UTILS_C) | $(BIN_DIR)
	@echo "PiControl: Compiling test utils $@"
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -I$(TEST_DIR) -I$(SRC_DIR) -c -fPIC $^ -o $@
	
$(BIN_TEST_DIR)/%_test: $(TEST_DIR)/%_test.o $(SRC_DIR)/%.o | $(TEST_UTILS_LIB)
	@echo "PiControl: Compiling test $@"
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) -L$(dir $|) -o $@ $^ -l:$(notdir $|)

$(TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@echo "PiControl: Compiling source object for test $@"
	$(CC) $(CFLAGS) -I$(SRC_DIR) -I$(TEST_DIR) -c $< -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "PiControl: Compiling source object $@"
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

$(BIN_DIR)::
	@[ -d "$@" ] || mkdir -p "$@"
