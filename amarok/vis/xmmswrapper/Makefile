CC = g++
CFLAGS = `xmms-config --cflags` -O3
LFLAGS = `gtk-config --libs` `xmms-config --libs`

BIN=amK_xmmswrapper
all: $(BIN)

$(BIN): xmmswrapper.cpp
	$(CC) -o $(BIN) $(CFLAGS) $(LFLAGS) xmmswrapper.cpp

clean:
	rm -f *.o *.so* *~ $(BIN)
