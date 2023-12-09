SRC_DIR  := src
OBJ_DIR  := obj
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
	$(RM) -rv $(BIN_DIR) $(OBJ_DIR) $(SRC_DIR)/**/*.o $(SRC_DIR)/**/*.d $(TEST)

$(EXE): $(OBJ_DIR)/picontrol.o $(OBJ_DIR)/picontrol_uinput.o | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS) -Isrc

$(SERVER): $(OBJ_DIR)/picontrol_server.o $(OBJ_DIR)/picontrol_iputils.o | $(BIN_DIR)
	$(CC) $^ -o $@ -lxdo $(LDFLAGS) -Isrc

$(TEST): $(SRC_DIR)/data_structures/ring_buffer.o $(SRC_DIR)/data_structures/test_ring_buffer.o | $(TEST_DIR)
	$(CC) $^ -o $@ $(LDFLAGS) -Isrc/data_structures

%.o: %.c
	mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@
