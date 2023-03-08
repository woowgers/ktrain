SOURCES := src/main.c src/keep_event_loop.c src/util.c src/wbuffer.c src/csi_functions.c
CFLAGS := -g3 -O3 -nostdlib

ktrain: $(SOURCES)
	$(CC) -o ktrain $(SOURCES) $(CFLAGS)


texts:
	mkdir -p /usr/share/ktrain
	cp -nt /usr/share/ktrain text/*.txt

install: ktrain texts
	cp -t /usr/bin ktrain

local_install: ktrain texts
	cp -t /usr/local/bin ktrain
