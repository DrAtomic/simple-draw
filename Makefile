RAY_DIR = ./deps/raylib-5.0/src

INCLUDE_PATHS = -I.
INCLUDE_PATHS += -I$(RAY_DIR)

APP = draw

SOURCES = main.c
PLUG_SOURCES = plug.c arena.c
PLUG_INCLUDES = plug.h hbb_circular_queue.h arena.h

CFLAGS = -Wall -Wextra -g
LIBS = -L./$(RAY_DIR)
LINK_OPTS = -l:libraylib.so -lm -ldl -Wl,-rpath=$(RAY_DIR) -Wl,-rpath=.

all: $(APP)

$(RAY_DIR)/libraylib.so:
	make -C $(RAY_DIR) RAYLIB_LIBTYPE=SHARED

libplug.so: $(PLUG_SOURCES) $(PLUG_INCLUDES)
	gcc $(CFLAGS) $(INCLUDE_PATHS) -fPIC -shared $(PLUG_SOURCES) -o libplug.so $(LIBS) $(LINK_OPTS)

$(APP): $(RAY_DIR)/libraylib.so libplug.so main.c
	gcc $(CFLAGS) $(INCLUDE_PATHS) $(SOURCES) -o $(APP) $(LIBS) $(LINK_OPTS)

clean:
	make -C $(RAY_DIR) clean
	rm $(APP) *.so
