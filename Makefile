################################## Variables ###################################
SRC_DIR    := src
BIN_DIR    := bin
TEST_DIR   := tst

EXE        := $(BIN_DIR)/picontrol
SERVER     := $(BIN_DIR)/picontrol_server

CC         := gcc
CFLAGS     := -O3 -MMD -MP
LDFLAGS    :=
LDLIBS     :=
LDFLAGS    := $(if $(strip $(LDFLAGS)),-l $(LDFLAGS),)
LDLIBS     := $(if $(strip $(LDLIBS)),-l $(LDLIBS),)

TEST_FILES := $(shell find $(SRC_DIR)/ -type f -name test_\*.c)
TEST_TARGETS := $(basename $(TEST_FILES))

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG
endif


################################ Phony Targets #################################
.PHONY: all
all: picontrol server test

.PHONY: picontrol
picontrol: $(EXE)

.PHONY: server
server: $(SERVER)

.PHONY: test
test: $(TEST_TARGETS)

.PHONY: clean
clean:
	$(RM) -r $(BIN_DIR)/
	$(RM) $(TEST_TARGETS)
	find $(SRC_DIR)/ -type f \( -name \*.o -o -name \*.d \) | xargs -r rm


################################### Targets ####################################
$(EXE): $(SRC_DIR)/picontrol.o $(SRC_DIR)/picontrol_uinput.o | $(BIN_DIR)
	@echo "PiControl: Making $(EXE)"
	$(CC) $^ -o $@ $(LDFLAGS) -I$(SRC_DIR)

$(SERVER): $(SRC_DIR)/picontrol_server.o $(SRC_DIR)/picontrol_iputils.o | $(BIN_DIR)
	@echo "PiControl: Making $(SERVER)"
	$(CC) $^ -o $@ -lxdo $(LDFLAGS) -I$(SRC_DIR)

.SECONDEXPANSION:
$(TEST_TARGETS): $$(addsuffix .c,$$@ $$(@D)/$$(subst test_,,$$(notdir $$@)))
	@echo "PiControl: Compiling test $@"
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) $^ -o $@ $(CFLAGS) -I$(SRC_DIR)

%.o: %.c
	@[ -d "$(@D)" ] || mkdir -p "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@
