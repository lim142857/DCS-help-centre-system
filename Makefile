PORT=53356
CFLAGS= -DPORT=$(PORT) -g -Wall -std=c99

all:  hcq_server 

hcq_server: hcq_server.o  socket.o hcq.o
	gcc $(CFLAGS) -o hcq_server hcq_server.o socket.o hcq.o

hcq_server.o: hcq_server.c hcq_server.h socket.h
	gcc $(CFLAGS) -c hcq_server.c

%.o: %.c %.h
	gcc ${CFLAGS} -c $<

clean: 
	rm helpcentre *.o
