EXEC = ppos

CC = gcc
CFLAGS = -Wall -g

OBJS = pingpong-contab-prio.o ppos_core.o queue.o

HEADERS = ppos.h ppos_data.h queue.h

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o

purge: clean
	rm -f $(EXEC)
