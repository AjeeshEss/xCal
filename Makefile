CC = gcc
CFLAGS = -Wall -std=c11 -g -DNDEBUG -fPIC `pkg-config --cflags python3`
LDFLAGS = 

all: caltool Cal.so

caltool: calutil.o caltool.o

calutil.o: calutil.c calutil.h

caltool.o: caltool.c caltool.h calutil.h

calmodule.o: calmodule.c calutil.h

Cal.so: calmodule.o calutil.o
	$(CC) -shared calmodule.o calutil.o -o Cal.so

calmodule.o: calmodule.c calutil.h

clean:
	rm -f *.o *.so caltool
