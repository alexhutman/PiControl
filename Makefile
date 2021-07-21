CC     = gcc
CFLAGS = -O2
LIBS   = xdo

ifdef DEBUG
	override CFLAGS += -DPI_CTRL_DEBUG
endif

OBJS   = picontrol.o \
       #other objs here

NAME   = picontrol
SERVER = picontrol_server

.PHONY: test_xdo
test_xdo: $(NAME).o
	$(CC) -o $(NAME) $(NAME).o -l $(LIBS)

.PHONY: test_server
test_server: $(NAME)_iputils.o $(SERVER).o
	$(CC) -o $(SERVER) $^ -l $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o $(NAME) $(SERVER)
