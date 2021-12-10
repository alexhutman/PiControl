CC             = gcc
CFLAGS         = -O3
LIBS           =
LIBS_FLAG_ARGS := $(if $(strip $(LIBS)),-l $(LIBS),)

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG
endif

OBJS   = picontrol.o \
       #other objs here

NAME   = picontrol
SERVER = picontrol_server

test_xdo: $(NAME)_uinput.o $(NAME).o
	$(CC) -o $(NAME) $^ $(LIBS_FLAG_ARGS)

test_server: $(NAME)_iputils.o $(SERVER).o
	$(CC) -o $(SERVER) $^ $(LIBS_FLAG_ARGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o $(NAME) $(SERVER)
