SOURCES := src/main.c src/keep_event_loop.c src/util.c src/wbuffer.c src/csi_functions.c
CFLAGS := -g3


all: main

main: $(SOURCES)
	$(CC) -o ktrain $(SOURCES) $(CFLAGS)


config:
	sudo mkdir -p ~/.config/ktrain
	sudo cp -nt ~/.config/ktrain text/*.txt

_install: main
	sudo cp -ft /usr/bin ktrain

_local_install: ktrain
	sudo cp -ft /usr/local/bin ktrain

install: _install config

local_install: _local_install config

uninstall:
	sudo rm /usr/bin/ktrain

local_uninstall:
	sudo rm /usr/local/bin/ktrain
