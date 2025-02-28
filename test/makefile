CC = gcc
CFLAGS = -lSDL2 -lSDL2_ttf -lpthread

all: simulator traffic_generator receiver

simulator: main.c simulator.c queue.c
	$(CC) main.c simulator.c queue.c -o simulator $(CFLAGS)

traffic_generator: traffic_generator.c
	$(CC) traffic_generator.c -o traffic_generator

receiver: receiver.c
	$(CC) receiver.c -o receiver

clean:
	rm -f simulator traffic_generator receiver
