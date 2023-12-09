SRC_DIR  := src
BIN_DIR  := bin
TEST_DIR := tst

EXE      := $(BIN_DIR)/picontrol
SERVER   := $(BIN_DIR)/picontrol_server
TEST     := $(TEST_DIR)/test_ring_buffer

CC       := gcc
CFLAGS   := -O3 -MMD -MP
LDFLAGS  :=
LDLIBS   :=
LDFLAGS  := $(if $(strip $(LDFLAGS)),-l $(LDFLAGS),)
LDLIBS   := $(if $(strip $(LDLIBS)),-l $(LDLIBS),)

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG
endif

.PHONY: all
all: picontrol server test

.PHONY: picontrol
picontrol: $(EXE)

.PHONY: server
server: $(SERVER)

.PHONY: test
test: $(TEST)

.PHONY: clean
clean:
	$(RM) -r $(BIN_DIR) $(TEST)
	find $(SRC_DIR)/ -type f \( -name \*.o -o -name \*.d \) | xargs rm

$(EXE): $(SRC_DIR)/picontrol.o $(SRC_DIR)/picontrol_uinput.o | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS) -I$(SRC_DIR)

$(SERVER): $(SRC_DIR)/picontrol_server.o $(SRC_DIR)/picontrol_iputils.o | $(BIN_DIR)
	$(CC) $^ -o $@ -lxdo $(LDFLAGS) -I$(SRC_DIR)

$(TEST): $(SRC_DIR)/data_structures/ring_buffer.o $(SRC_DIR)/data_structures/test_ring_buffer.o | $(TEST_DIR)
	$(CC) $^ -o $@ $(LDFLAGS) -I$(SRC_DIR)/data_structures

%.o: %.c
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p "$@"
