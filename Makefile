SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE     := $(BIN_DIR)/picontrol
SERVER  := $(BIN_DIR)/picontrol_server
OBJ     := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

CC      := gcc
CFLAGS  := -O3 -Iinclude -MMD -MP
LDFLAGS :=
LDLIBS  :=
LDFLAGS := $(if $(strip $(LDFLAGS)),-l $(LDFLAGS),)
LDLIBS  := $(if $(strip $(LDLIBS)),-l $(LDLIBS),)

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG
endif

.PHONY: all
all: picontrol server

.PHONY: picontrol
picontrol: $(EXE)

.PHONY: server
server: $(SERVER)

.PHONY: clean
clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

$(EXE): $(OBJ_DIR)/picontrol.o $(OBJ_DIR)/picontrol_uinput.o | $(BIN_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)

$(SERVER): $(OBJ_DIR)/picontrol_server.o $(OBJ_DIR)/picontrol_iputils.o | $(BIN_DIR)
	$(CC) $^ -o $@ -lxdo $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

-include $(OBJ:.o=.d)
