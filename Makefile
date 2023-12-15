CC   = gcc
OPTS = -Wall

all: udp library server client

# this generates the target executables

udp:	
	$(CC) -fpic -c udp.c -o udp.o
	$(CC) -shared -o libudp.so udp.o	

library:
	$(CC) -fpic -c -o mfs.o mfs.c -Wall -ludp -L.
#	$(CC) -o mfs.o mfs.o udp.o  
	$(CC) -shared -o libmfs.so mfs.o udp.o
#	$(CC) -lmfs -L. try.c -o out.o
#	$(CC) -lmfs -L. -o server server.o udp.o
#	$(CC) -lmfs -L. -o client client.o udp.o

server: server.o udp.o
	$(CC) $(OPTS) -c server.c -o server.o -lmfs -L.
	$(CC)  -o server server.o udp.o -lmfs -L.

client: client.o udp.o
	$(CC) $(OPTS) -c client.c -o client.o -lmfs -L.
	$(CC)  -o client client.o udp.o -lmfs -L.

	$(CC) $(OPTS) -c client_example_1.c -o client_example_1.o -lmfs -L.
	$(CC)  -o client_example_1 client_example_1.o udp.o -lmfs -L.
# this is a generic rule for .o files 
#%.o: %.c 
#	$(CC) $(OPTS) -c $< -o $@ -lmfs -L.

clean:
	rm -f server.o udp.o client.o server client libmfs.so mfs.o out.o
	

