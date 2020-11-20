CC = gcc
LIBS = xdo

OBJS = picontrol.o \
       #other objs here
NAME = picontrol

.PHONY: clean

$(NAME): $(OBJS)
	$(CC) -o $@ $< -l $(LIBS)

%.o: %.c
	$(CC) -c $<

clean:
	rm -f *.o $(NAME)
