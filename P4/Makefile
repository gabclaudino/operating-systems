EXEC = pingpong-scheduler

CC = gcc
CFLAGS = -Wall -g

OBJS = pingpong-scheduler.o ppos_core.o queue.o

HEADERS = ppos.h ppos_data.h queue.h

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(EXEC)
