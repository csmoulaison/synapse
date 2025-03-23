// STL - do NOT include stdbool as it seems to conflict with xlib in very hard
// to diagnose ways!
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Game
#define PANIC() printf("PANIC at %s:%u\n", __FILE__, __LINE__); exit(1)

#include <cglm/cglm.h>
#include "linalg.c"
#include "lerp.c"
#include "input.c"
#include "render_group.c"
#include "game.c"

// Platform
#include <GL/gl3w.h>
#include <GL/glx.h>
#include <X11/extensions/Xfixes.h>
#include "gl_shaders.c"
#include "x11_structs.c"
#include "x11_init.c"
#include "x11_rendergl.c"
#include "x11_loop.c"

int32_t main(int32_t argc, char** argv) {
	struct x11_context x11 = x11_init();
	x11_loop(&x11);
	return 0;
}
