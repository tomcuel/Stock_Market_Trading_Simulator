CC=g++ -std=c++17
CGFLAGS=-Wall -Wfatal-errors 

all : server.x client.x launcher.x

%.x : %.o 
	$(CC) $(CGFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CGFLAGS) -o $@ -c $< 

clean:
	rm -f *.o

realclean: clean
	rm -f *.x