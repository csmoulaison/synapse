#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl3w.h>
#include <GL/glx.h>
#include <X11/extensions/Xfixes.h>

#include "cglm/cglm.h"

#include "typedefs.h"

#define logerr(msg) printf(msg)
#define log(msg)    printf(msg)

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

struct gl_render_state {
    // For change over time
    u64 clock;

    // GL handles
	u32 shader_program; 
	u32 vertex_array;
	u32 element_buffer;

	// X Resources
	Display* display;
	Window window;

	// Camera state
	v3 cam_pos;
	v3 cam_target;
	v3 cam_up;
	float cam_yaw;
	float cam_pitch;
};

u32 compile_and_create_shader(const char* src, GLenum type) {
    u32 shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, 0);
	glCompileShader(shader);

	i32 success;
	char inf[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if(!success)
	{
    	glGetShaderInfoLog(shader, 512, NULL, inf);
    	printf("Shader compilation failed: %s\n", inf);
    	exit(1);
	}

	return shader;
}

void gl_render(struct gl_render_state state) 
{
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(state.shader_program);

	// Model matrix
	m4 model = GLM_MAT4_IDENTITY_INIT;
	//glm_rotate(model, (f32)state.clock / 50.0f, (v3){1.0f, 0.0f, 0.0f});

	u32 model_loc = glGetUniformLocation(state.shader_program, "model");
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, model[0]);

	// View matrix
	m4 view = GLM_MAT4_IDENTITY_INIT;
	vec3 cam_pos = {0.0f, 0.0f, 3.0f};
    vec3 cam_target = {0.0f, 0.0f, 0.0f};
    vec3 cam_up = {0.0f, 1.0f, 0.0f};
	glm_lookat(
    	/*
    	state.cam_pos,
    	state.cam_target,
    	state.cam_up,
    	*/
    	cam_pos,
    	cam_target,
    	cam_up,
    	view);
	u32 view_loc = glGetUniformLocation(state.shader_program, "view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, view[0]);

	// Projection matrix
	m4 projection;
	// TODO - second param should be width/height.
	glm_perspective(glm_rad(45.0f), 1, 0.1f, 100.0f, projection);

	u32 projection_loc = glGetUniformLocation(state.shader_program, "projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, projection[0]);

	glBindVertexArray(state.vertex_array);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.element_buffer);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glXSwapBuffers(state.display, state.window);
}

i32 main(i32 argc, char** argv)
{
    // Connect to X server
	Display* display = XOpenDisplay(0);
	if(!display) 
	{
    	logerr("Failure on XOpenDisplay.\n");
    	exit(1);
	}

	// Request a suitable framebuffer config
	GLXFBConfig framebuf_conf;
	{
        i32 visual_attribs[] = 
        {
            GLX_X_RENDERABLE,   True,
            GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
            GLX_RENDER_TYPE,    GLX_RGBA_BIT,
            GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
            GLX_RED_SIZE,	    8,
            GLX_GREEN_SIZE,	    8,
            GLX_BLUE_SIZE,	    8,
            GLX_ALPHA_SIZE,	    8,
            GLX_DEPTH_SIZE,	    24,
            GLX_STENCIL_SIZE,   8,
            GLX_DOUBLEBUFFER,   True,
        	GLX_SAMPLE_BUFFERS, 1,
        	GLX_SAMPLES,		4
        };

    	i32 confs_len;
    	GLXFBConfig* framebuf_confs = glXChooseFBConfig(
        	display, 
        	DefaultScreen(display), 
        	visual_attribs, 
        	&confs_len);
    	if(!framebuf_confs) 
        {
    		log("Failure on glXChooseFBConfig.\n");
    		exit(1);
    	}

    	// TODO - get conf with most samples per pixel per this example:
       	// https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)
       	framebuf_conf = framebuf_confs[0];

    	XFree(framebuf_confs);
	}

	// Create X colormap and window per framebuffer config
	Window window;
	{
    	XVisualInfo* visual_info = glXGetVisualFromFBConfig(display, framebuf_conf);

    	XSetWindowAttributes set_win_attribs;
		set_win_attribs.colormap = XCreateColormap(
    		display,
    		RootWindow(display, visual_info->screen),
    		visual_info->visual,
    		AllocNone);
    	set_win_attribs.background_pixmap = None;
    	set_win_attribs.border_pixel = 0;
    	set_win_attribs.event_mask = 
    	StructureNotifyMask 
    	| ExposureMask 
    	| KeyPressMask 
    	| KeyReleaseMask 
    	| PointerMotionMask;

    	window = XCreateWindow(
        	display,
        	RootWindow(display, visual_info->screen),
        	0,
        	0,
        	100,
        	100,
        	0,
        	visual_info->depth,
        	InputOutput,
        	visual_info->visual,
        	CWBorderPixel | CWColormap | CWEventMask,
        	&set_win_attribs);
    	if(!window)
    	{
        	printf("Failure on XCreateWindow.\n");
        	exit(1);
    	}

    	XFree(visual_info);
	}

	// Name and map window to X
	XStoreName(display, window, "GLX Window");
	XMapWindow(display, window);

	// Check for required extension for GL 3.0.
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
	{
    	const char *gl_exts = glXQueryExtensionsString(display, DefaultScreen(display));
    	glXCreateContextAttribsARB = 
        	(glXCreateContextAttribsARBProc)
        	glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");

    	// Pulled from helper function at the same khronos doc mentioned above.
    	const char *extension = "GLX_ARB_create_context";
        const char *start;
        const char *where, *terminator;
          
        // Extension names should not have spaces.
        where = strchr(extension, ' ');
        if (where || *extension == '\0')
        {
            printf("Extension name has a space.\n");
            exit(1);
        }

        // It takes a bit of care to be fool-proof about parsing the OpenGL
        // extensions string. Don't be fooled by sub-strings, etc.
        i32 found_ext = 0;
        for (start=gl_exts;;) 
        {
        	where = strstr(start, extension);
            if (!where)
            {
            	break;
            }

            terminator = where + strlen(extension);

            if ( where == start || *(where - 1) == ' ' )
            {
                if ( *terminator == ' ' || *terminator == '\0' )
                {
                    found_ext = 1;
                }

         		start = terminator;
            }
        }
        if(!found_ext) 
        {
    		printf("Extension not found.\n");
    		exit(1);
        }
	}

	// Create GLX context and window
	i32 ctx_attribs[] = 
	{
    	GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    	GLX_CONTEXT_MINOR_VERSION_ARB, 6,
    	None
	};

	GLXContext ctx = glXCreateContextAttribsARB(
    	display, 
    	framebuf_conf, 
    	0, 
    	1, 
    	ctx_attribs);
	if(!glXIsDirect(display, ctx))
	{
    	printf("Obtained indirect GLX rendering context.\n");
    	exit(1);
	}

	// Bind GLX to window
	glXMakeCurrent(display, window, ctx);

	// Init gl3w for core profile commands
	if(gl3wInit()) {
    	printf("Failure on gl3wInit.\n");
    	exit(1);
	}

	// Init rendering state

	// Compile frag and vertex shaders
	const char* vert_src =
	"#version 330 core\n"
	"layout (location = 0) in vec3 a_pos;\n"
	"layout (location = 1) in vec3 b_pos;\n"
	"layout (location = 2) in float orientation;\n"

	"uniform mat4 projection;\n"
	"uniform mat4 model;\n"
	"uniform mat4 view;\n"

	"out vec3 my_color;\n"

	"void main()\n"
	"{\n"
		"mat4 proj_view_model = projection * view * model;\n"
		"vec4 a_proj = proj_view_model * vec4(a_pos, 1.0);\n"
		"vec4 b_proj = proj_view_model * vec4(b_pos, 1.0);\n"

		"vec2 a_screen = a_proj.xy / a_proj.w;\n"
		"vec2 b_screen = b_proj.xy / b_proj.w;\n"

		"vec2 dir = normalize(b_screen - a_screen);\n"
		"vec2 normal = vec2(-dir.y, dir.x);\n"

		"float width = 0.006;\n"
		"normal *= width / 2.0;\n"

		"vec4 offset = vec4(normal * orientation * a_proj.z, 0.0, 0.0);\n"
		"gl_Position = a_proj + offset;\n"
		"my_color = mix(vec3(0.1, 0.1, 0.1), vec3(0.7, 0.7, 0.7), clamp(a_proj.z / 6, 0, 1));\n"
	"}\0";
	u32 vert_shader = compile_and_create_shader(vert_src, GL_VERTEX_SHADER);

	const char* frag_src =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"in vec3 my_color;\n"
	"void main()\n"
	"{\n"
	"	FragColor = vec4(my_color, 1.0f);\n"
	"}\0";
	u32 frag_shader = compile_and_create_shader(frag_src, GL_FRAGMENT_SHADER);

	u32 shader_prog = glCreateProgram();
	glAttachShader(shader_prog, vert_shader);
	glAttachShader(shader_prog, frag_shader);
	glLinkProgram(shader_prog);

	// Cleanup shaders after linking
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	float verts[] =
	{
    	// Line segment a, b - each position is repeated so that each can be pushed
    	// along the normal in opposite directions.
    	 0.5f,  0.5f,  0.5f,  -0.5f, -0.5f, -4.5f,  -1,
    	 0.5f,  0.5f,  0.5f,  -0.5f, -0.5f, -4.5f,   1,
    	-0.5f, -0.5f, -4.5f,   0.5f,  0.5f,  0.5f,  -1,
    	-0.5f, -0.5f, -4.5f,   0.5f,  0.5f,  0.5f,   1
	};

	u32 vert_array;
	glGenVertexArrays(1, &vert_array);
	glBindVertexArray(vert_array);

	u32 vert_buffer;
	glGenBuffers(1, &vert_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vert_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	u32 indices[] =
	{
    	0, 1, 3,
    	1, 2, 3
	};

	u32 element_buffer;
	glGenBuffers(1, &element_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// A position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(f32), (void*)0);
	glEnableVertexAttribArray(0);

	// B position attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(f32), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Normal direction attribute
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(f32), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Construct gl_render_state
	struct gl_render_state state = (struct gl_render_state)
	{
    	0.0f, 
    	shader_prog, 
    	vert_array, 
    	element_buffer, 
    	display, 
    	window, 
    	{0.0f, 0.0f, 3.0f}, 
    	{0.0f, 0.0f, 0.0f},
    	{0.0f, 1.0f, 0.0f},
    	-90.0f,
    	0.0f
	};

	u8 left = 0;
	u8 down = 0;
	u8 up = 0;
	u8 right = 0;

	u8 mouse_moved_yet = 0;
	u8 mouse_just_warped = 0;

	i32 mouse_x;
	i32 mouse_y;

	// Lock mouse cursor and 
	XGrabPointer(state.display, state.window, 1, PointerMotionMask, GrabModeAsync, GrabModeAsync, window, None, CurrentTime);
	XFixesHideCursor(display, window);
	XSync(display, 1);

	// Event loop
	i32 running = 1;
	while(running) 
	{
		i32 mouse_delta_x = 0;
		i32 mouse_delta_y = 0;
    
		state.clock++;
    	while(XPending(display)) 
    	{
        	XEvent xevent;
        	XNextEvent(state.display, &xevent);
			switch(xevent.type) 
			{
    			case Expose:
        		{
            		gl_render(state);
            		break;
        		}
        		case ConfigureNotify:
        		{
                	XWindowAttributes win_attribs;
                	XGetWindowAttributes(state.display, state.window, &win_attribs);
            		glViewport(0, 0, win_attribs.width, win_attribs.height);
            		break;
        		}
            	case MotionNotify:
                {
                	if(mouse_just_warped) {
                    	mouse_just_warped = 0;
                    	break;
                	}

					if(!mouse_moved_yet) {
						mouse_delta_x = 0;
						mouse_delta_y = 0;
						mouse_x = xevent.xmotion.x;
						mouse_y = xevent.xmotion.y;
						mouse_moved_yet = 1;
					} else {
						mouse_delta_x = xevent.xmotion.x - mouse_x;
						mouse_delta_y = xevent.xmotion.y - mouse_y;
						mouse_x = xevent.xmotion.x;
						mouse_y = xevent.xmotion.y;
					}

                	XWindowAttributes win_attribs;
                	XGetWindowAttributes(state.display, xevent.xmotion.window, &win_attribs);

					i8 inner_bounds = 4;
					if(mouse_x < inner_bounds
					|| mouse_x > win_attribs.width - inner_bounds
					|| mouse_y < inner_bounds
					|| mouse_y > win_attribs.height - inner_bounds) {
    					mouse_just_warped = 1;
    					mouse_x = win_attribs.width / 2;
    					mouse_y = win_attribs.height / 2;

    					XWarpPointer(
        					state.display, 
        					None,
        					xevent.xmotion.window,
        					0, 0, 0, 0,
							win_attribs.width / 2, win_attribs.height / 2);
    					XSync(state.display, 1);
					}
					break;
				}
        		case KeyPress:
        		{
                	if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_Escape))
            		{
                		running = 0;
            		}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_A))
                	{
						left = 1;
                	}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_S))
                	{
						down = 1;
                	}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_W))
                	{
						up = 1;
                	}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_D))
                	{
						right = 1;
                	}
                	break;
        		}
        		case KeyRelease:
        		{
                	if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_A))
                	{
						left = 0;
                	}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_S))
                	{
						down = 0;
                	}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_W))
                	{
						up = 0;
                	}
                	else if(xevent.xkey.keycode == XKeysymToKeycode(state.display, XK_D))
                	{
						right = 0;
                	}
                	break;
        		}
        		default:
        		{
            		break;
        		}
			}
    	}

    	v3 cam_forward;
    	cam_forward[0] = cos(glm_rad(state.cam_yaw)) * cos(glm_rad(state.cam_pitch));
    	cam_forward[1] = sin(glm_rad(state.cam_pitch));
    	cam_forward[2] = sin(glm_rad(state.cam_yaw)) * cos(glm_rad(state.cam_pitch));
    	glm_vec3_normalize(cam_forward);

		f32 speed = 0.1f;

		v3 forward_speed;
		glm_vec3_scale(cam_forward, speed, forward_speed);

		v3 right_speed;
		glm_vec3_cross(cam_forward, state.cam_up, right_speed);
		glm_vec3_normalize(right_speed);
		glm_vec3_scale(right_speed, speed, right_speed);

    	if(left) {
			glm_vec3_sub(state.cam_pos, right_speed, state.cam_pos);
    	}
    	if(down) {
			glm_vec3_sub(state.cam_pos, forward_speed, state.cam_pos);
    	}
    	if(up) {
			glm_vec3_add(state.cam_pos, forward_speed, state.cam_pos);
    	}
    	if(right) {
			glm_vec3_add(state.cam_pos, right_speed, state.cam_pos);
    	}

		f32 look_mod = 25.0f;
    	state.cam_yaw += (float)mouse_delta_x   / look_mod;
    	state.cam_pitch -= (float)mouse_delta_y / look_mod;

    	// Set cam target
    	glm_vec3_add(state.cam_pos, cam_forward, state.cam_target);

		gl_render(state);
	}

	return 0;
}
