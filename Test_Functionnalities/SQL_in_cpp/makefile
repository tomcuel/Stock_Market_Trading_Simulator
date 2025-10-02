CC=g++ -std=c++17
CGFLAGS= -Wall -Wfatal-errors
LDLIBS=-L/opt/homebrew/lib -lsqlite3

all: test_sql.x

test_sql.x: test_sql.o
	$(CC) $(CGFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CC) $(CGFLAGS) -o $@ -c $< $(LDLIBS)

clean:
	rm -f *.o

realclean: clean
	rm -f test_sql.x