#define MEMORY_POOL_BYTES 1073741824

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

uint32_t compile_shader(const char* src, GLenum type) 
{
    uint32_t shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, 0);
	glCompileShader(shader);

	int32_t success;
	char inf[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if(!success)
	{
    	glGetShaderInfoLog(shader, 512, NULL, inf);
    	PANIC();
	}

	return shader;
}

struct x11_context x11_init()
{
    struct x11_context x11;

    x11.display = XOpenDisplay(0);
	if(!x11.display)
	{
    	PANIC();
	}

	// Get a suitable framebuffer configuration
	int32_t desired_fb_attribs[] =
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
    	GLX_SAMPLES,		4,
    	None
	};
	int32_t configs_len;

	GLXFBConfig* fb_configs = glXChooseFBConfig(
    	x11.display,
    	DefaultScreen(x11.display),
    	desired_fb_attribs,
    	&configs_len);
	if(!fb_configs)
	{
    	PANIC();
	}
	
	// TODO - get configuration with the most samples per pixel, for instance.
	GLXFBConfig fb_config = fb_configs[0];
	XFree(fb_configs);

	// Query visual info from the framebuffer config to be used in window creation
	XVisualInfo* visual_info = glXGetVisualFromFBConfig(x11.display, fb_config);
	Window win_root = RootWindow(x11.display, visual_info->screen);

	// Window attributes to set
	XSetWindowAttributes win_attribs;
	win_attribs.colormap = XCreateColormap(
    	x11.display,
    	win_root,
    	visual_info->visual,
    	AllocNone);
	win_attribs.background_pixmap = None;
	win_attribs.border_pixel = 0;
	win_attribs.event_mask =
    	StructureNotifyMask |
    	ExposureMask |
    	KeyPressMask |
    	KeyReleaseMask |
    	PointerMotionMask;

    // Create window
    x11.winw = 100;
    x11.winh = 100;
	x11.window = XCreateWindow(
    	x11.display,
    	win_root,
    	0,
    	0,
    	x11.winw,
    	x11.winh,
    	0,
    	visual_info->depth,
    	InputOutput,
    	visual_info->visual,
    	CWBorderPixel | CWColormap | CWEventMask,
    	&win_attribs);
	if(!x11.window) 
	{
    	PANIC();
	}

	XFree(visual_info);

	XStoreName(x11.display, x11.window, "Synapse");
	XMapWindow(x11.display, x11.window);

	// Check for required extension for GL 3.0.
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
	const char *gl_exts = glXQueryExtensionsString(x11.display, DefaultScreen(x11.display));
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
        PANIC();
    }

    // It takes a bit of care to be fool-proof about parsing the OpenGL
    // extensions string. Don't be fooled by sub-strings, etc.
    int32_t found_ext = 0;
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
        PANIC();
    }

	// Create GLX context and window
	int32_t glx_attribs[] = 
	{
    	GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
    	GLX_CONTEXT_MINOR_VERSION_ARB, 6,
    	None
	};

	GLXContext glx = glXCreateContextAttribsARB(
    	x11.display, 
    	fb_config, 
    	0, 
    	1, 
    	glx_attribs);
	if(!glXIsDirect(x11.display, glx))
	{
    	PANIC();
	}

	// Bind GLX to window
	glXMakeCurrent(x11.display, x11.window, glx);

	// Lock and hide mouse cursor
	XGrabPointer(x11.display, x11.window, 1, PointerMotionMask, GrabModeAsync, GrabModeAsync, x11.window, None, CurrentTime);
	XFixesHideCursor(x11.display, x11.window);
	XSync(x11.display, 1);

	// Init gl3w for core profile commands
	if(gl3wInit()) 
    {
        PANIC();
	}

	// Create shader program
	uint32_t vert_shader = compile_shader(vertex_shader_src, GL_VERTEX_SHADER);
	uint32_t frag_shader = compile_shader(frag_shader_src, GL_FRAGMENT_SHADER);

	x11.gl.shader_program_lines = glCreateProgram();
	glAttachShader(x11.gl.shader_program_lines, vert_shader);
	glAttachShader(x11.gl.shader_program_lines, frag_shader);
	glLinkProgram(x11.gl.shader_program_lines);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);

	// Create vert array and buffer objects from line_verts data:
	glGenVertexArrays(1, &x11.gl.vertex_array_object);
	glBindVertexArray(x11.gl.vertex_array_object);

	// Columns: line start v3 - line end v3 - normal direction (alternately -1, 1)
	// Rows: a with -1, a with 1, b with -1, b with 1
	// The normal direction is used to create the line thickness in either direction.
	float line_vertices[] = 
	{
    	// TODO - define identity line to be transformed per each v3_line
    	 0.0f,  0.0f,  0.0f,   1.0f,  1.0f,  1.0f,  -1, 0,
    	 0.0f,  0.0f,  0.0f,   1.0f,  1.0f,  1.0f,   1, 0,
    	 1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  0.0f,  -1, 1,
    	 1.0f,  1.0f,  1.0f,   0.0f,  0.0f,  0.0f,   1, 1
	};
	uint32_t vertex_buffer_object;
	glGenBuffers(1, &vertex_buffer_object);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
	glBufferData(
    	GL_ARRAY_BUFFER, 
    	sizeof(line_vertices), 
    	line_vertices, 
    	GL_STATIC_DRAW);

	uint32_t line_indices[] =
	{
    	0, 1, 3,
    	1, 2, 3
	};
	uint32_t element_buffer_object;
	glGenBuffers(1, &element_buffer_object);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_object);
	glBufferData(
    	GL_ELEMENT_ARRAY_BUFFER, 
    	sizeof(line_indices), 
    	line_indices, 
    	GL_STATIC_DRAW);

	// Bind vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(3);

	// Initialize input to 0
	// TODO - can we just do x11.input = {}?
	x11.input.mouse_delta_x = 0;
	x11.input.mouse_delta_y = 0;
	for(uint32_t i = 0; i < INPUT_BUTTONS_LEN; i++) 
	{
		x11.input.buttons[i].held = 0;
		x11.input.buttons[i].pressed = 0;
		x11.input.buttons[i].released = 0;
	}

	x11.mouse_moved_yet = 0;
	x11.mouse_just_warped = 0;

    if(clock_gettime(CLOCK_REALTIME, &x11.time_prev))
    {
        PANIC();
    }
    x11.time_since_start = 0;

	// TODO - raw memory page allocation
	x11.memory_pool = malloc(MEMORY_POOL_BYTES);
	x11.memory_pool_bytes = MEMORY_POOL_BYTES;

	game_init(x11.memory_pool, x11.memory_pool_bytes);
	
	return x11;
}
