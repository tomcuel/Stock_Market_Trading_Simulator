CC=g++ -std=c++17
CGFLAGS= -Wall -Wfatal-errors
LDLIBS= -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto

all: password_cryptage.x

password_cryptage.x: password_cryptage.o
	$(CC) $(CGFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.cpp
	$(CC) $(CGFLAGS) -o $@ -c $< $(LDLIBS)

clean:
	rm -f *.o

realclean: clean
	rm -f *.x