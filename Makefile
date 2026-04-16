include config.mk

LIBS = -lresolv -lssl -lcrypto
SOURCE = rsslib/rsslib.c util.c hnrss_reader.c main.c
TARGET = hnrss_reader

.PHONY: all clean test install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LIBS)

test:
	$(MAKE) -C rsslib test

clean:
	rm -f $(TARGET)
	$(MAKE) -C rsslib clean

install: 
	install $(TARGET) $(PREFIX)/bin/$(TARGET)
