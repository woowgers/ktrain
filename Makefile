SOURCES := src/main.c src/keep_event_loop.c src/util.c src/wbuffer.c src/csi_functions.c
CFLAGS := -g3

.PHONY: main

all: main

main: $(SOURCES)
	$(CC) -o ktrain $(SOURCES) $(CFLAGS)

local_install: ktrain
	cp -t /usr/local/bin ktrain
	mkdir ~/.config/ktrain
	cp -t ~/.config/ktrain text/*.txt
