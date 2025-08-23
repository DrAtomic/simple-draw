#include <sys/mman.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "raylib.h"
#include "plug.h"

const char *lib_plug_file_name = "libplug.so";
void *libplug;

plug_init_t plug_init;
plug_update_t plug_update;
plug_pre_reload_t plug_pre_reload;
plug_post_reload_t plug_post_reload;

Plug plug;

static void libplug_reload(void)
{
	if (libplug)
		dlclose(libplug);

	for (size_t attempt = 0; attempt < 5; attempt++) {
		libplug =  dlopen(lib_plug_file_name, RTLD_NOW);
		if (libplug != NULL)
			break;
		usleep(100000);
	}

	if (libplug == NULL) {
		fprintf(stderr, "failed to load libplug\n");
		exit(1);
	}
	plug_init = dlsym(libplug, "plug_init");
	if (plug_init == NULL) {
		fprintf(stderr, "failed to load plug_init\n");
		exit(1);
	}
	plug_update = dlsym(libplug, "plug_update");
	if (plug_update == NULL) {
		fprintf(stderr, "failed to load plug_update\n");
		exit(1);
	}
	plug_pre_reload = dlsym(libplug, "plug_pre_reload");
	if (plug_pre_reload == NULL) {
		fprintf(stderr, "failed to load plug_pre_reload\n");
		exit(1);
	}
	plug_post_reload = dlsym(libplug, "plug_post_reload");
	if (plug_post_reload == NULL) {
		fprintf(stderr, "failed to load plug_post_reload\n");
		exit(1);
	}
	printf("reloading!\n");
}

static int plug_should_reload(time_t *last_modified_time)
{
	int ret = 0;
	struct stat attr;

	if (stat(lib_plug_file_name, &attr)) {
		fprintf(stderr, "failed to stat %s\n", strerror(errno));
		exit(1);
	}

	if (attr.st_mtime != *last_modified_time) {
		*last_modified_time = attr.st_mtime;
		ret = 1;
	}

	return ret;
}

int main(void)
{
	plug.permanent_storage_size = Gigabytes(1);
	plug.permanent_storage = mmap(NULL, plug.permanent_storage_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, 0, 0);
	if (plug.permanent_storage == MAP_FAILED) {
		printf("buy more ram\n");
		exit(0);
	}

	libplug_reload();
	size_t factor = 80;
	time_t last_modified_time = {0};
	plug_should_reload(&last_modified_time);

	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN);
	InitWindow(factor*16, factor*9, "draw");
	SetTargetFPS(60);

	plug_init(&plug);
	while (!WindowShouldClose()) {
		if (plug_should_reload(&last_modified_time)) {
			plug_pre_reload(&plug);
			libplug_reload();
			plug_post_reload(&plug);
		}
		plug_update(&plug);
	}

	CloseWindow();

	return 0;
}
