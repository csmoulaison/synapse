#define MAX_RENDER_LINES 64

struct render_group 
{
	struct v3 clear_color;
    
	struct v3 cam_pos;
	struct v3 cam_target;

	struct v3_line lines[MAX_RENDER_LINES];
	struct v3_line line_colors[MAX_RENDER_LINES];
	uint16_t lines_len;
};
