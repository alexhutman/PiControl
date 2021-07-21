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

#$(NAME): $(NAME).o
#$(CC) -o $@ $< -l $(LIBS)

.PHONY: test_xdo
test_xdo: $(NAME).o
	$(CC) -o $(NAME) $(NAME).o -l $(LIBS)

#$(SERVER): $(SERVER).o
#$(CC) -o $@ $< -l $(LIBS)
.PHONY: test_server
test_server: $(SERVER).o
	$(CC) -o $(SERVER) $(SERVER).o -l $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o $(NAME) $(SERVER)
