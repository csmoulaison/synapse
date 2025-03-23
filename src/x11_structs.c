struct gl_context {
	uint32_t shader_program_lines;
	uint32_t vertex_array_object;
};

struct x11_context 
{
    Display* display;
	Window window;
	uint32_t winw;
	uint32_t winh;
    uint32_t mouse_moved_yet;
    uint32_t mouse_just_warped;

	float time_since_start;
	struct timespec time_prev;

	struct gl_context gl;
	struct render_group render_group;
	struct input_state input;

	void* memory_pool;
	uint32_t memory_pool_bytes;
};
