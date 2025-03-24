#define MAX_NETWORK_LINES 4
#define CAM_MOVE_SPEED 4.0f
#define CAM_MOVE_LERP_SPEED 16.0f
#define CAM_LOOK_LERP_SPEED 22.0f
#define CAM_LOOK_SPEED 6.0f

struct game_memory
{
	struct v3 cam_pos;
	struct v3 cam_pos_target;

	float cam_yaw;
	float cam_pitch;
	float cam_yaw_target;
	float cam_pitch_target;

	struct v3_line lines[MAX_NETWORK_LINES];

	float pulse;
};

void game_init(
    void* mem, 
    uint32_t mem_bytes)
{
    struct game_memory* game = (struct game_memory*)mem;

    game->cam_pos = v3_new(0.0f, 0.0f, 3.0f);
    game->cam_pos_target = v3_new(0.0f, 0.0f, 3.0f);
    game->cam_yaw = -90.0f;
    game->cam_pitch = 0.0f;
    game->cam_yaw_target = game->cam_yaw;
    game->cam_pitch_target = game->cam_pitch;

    game->lines[0].a = v3_new(0, 0, 0);
    game->lines[0].b = v3_new(0.7, 0.5, 0.7);

    game->lines[1].a = v3_new(0.7, 0.5, 0.7);
    game->lines[1].b = v3_new(1, -0.2, 1);

    game->lines[2].a = v3_new(1, -0.2, 1);
    game->lines[2].b = v3_new(2, -1.2, 1.5);

    game->lines[3].a = v3_new(1, -0.2, 1);
    game->lines[3].b = v3_new(0, -0.7, 2);

    game->pulse = 0;
}

// TODO - replace glm stuff with at least a wrapper
void game_update(
    struct render_group* render_group, 
    struct input_state* input, 
    float dt,
    void* mem, 
    uint32_t mem_bytes)
{
	struct game_memory* game = (struct game_memory*)mem;

	// Update camera look direction
	game->cam_yaw_target   += (float)input->mouse_delta_x * CAM_LOOK_SPEED * dt;
	game->cam_pitch_target -= (float)input->mouse_delta_y * CAM_LOOK_SPEED * dt;
	if(game->cam_pitch_target > 90.0f) 
	{
		game->cam_pitch_target = 90.0f;
	}
	else if(game->cam_pitch_target < -90.0f)
	{
		game->cam_pitch_target = -90.0f;
	}
	// TODO - wrap yaw. More complicated because of the lerping

	//if(game->cam_yaw != game->cam_yaw_target)
	if(true)
	{
		game->cam_yaw = lerp(game->cam_yaw, game->cam_yaw_target, dt * CAM_LOOK_LERP_SPEED);
		game->cam_pitch = lerp(game->cam_pitch, game->cam_pitch_target, dt * CAM_LOOK_LERP_SPEED);
	}
	else
	{
		game->cam_yaw_target = game->cam_yaw;
		game->cam_pitch_target = game->cam_pitch;
	}

	// Update game state
	// Update camera position
	// 
	// TODO - This is actually going to be cleaner once we just normalize the
	// final player speed, I think.
	struct v3 cam_forward = v3_new(
    	cos(radians(game->cam_yaw)) * cos(radians(game->cam_pitch)),
    	sin(radians(game->cam_pitch)),
    	sin(radians(game->cam_yaw)) * cos(radians(game->cam_pitch)));
	cam_forward = v3_normalize(cam_forward);

	struct v3 cam_up = v3_new(0.0f, 1.0f, 0.0f);
	struct v3 cam_right = v3_normalize(v3_cross(cam_forward, cam_up));

	if(input->move_forward.held) 
	{
		game->cam_pos_target = v3_add(game->cam_pos_target, v3_scale(cam_forward, CAM_MOVE_SPEED * dt));
	}
	if(input->move_back.held) 
    {
		game->cam_pos_target = v3_sub(game->cam_pos_target, v3_scale(cam_forward, CAM_MOVE_SPEED * dt));
	}
	if(input->move_left.held) 
	{
    	game->cam_pos_target = v3_sub(
        	game->cam_pos_target, 
        	v3_scale(cam_right, CAM_MOVE_SPEED * dt));
	}
	if(input->move_right.held) 
	{
    	game->cam_pos_target = v3_add(
        	game->cam_pos_target, 
        	v3_scale(cam_right, CAM_MOVE_SPEED * dt));
	}

	game->cam_pos.x = lerp(game->cam_pos.x, game->cam_pos_target.x, CAM_MOVE_LERP_SPEED * dt);
	game->cam_pos.y = lerp(game->cam_pos.y, game->cam_pos_target.y, CAM_MOVE_LERP_SPEED * dt);
	game->cam_pos.z = lerp(game->cam_pos.z, game->cam_pos_target.z, CAM_MOVE_LERP_SPEED * dt);

	if(input->bang_center.pressed)
	{
            game->cam_pos = v3_new(0.0f, 0.0f, 3.0f);
            game->cam_pos_target = v3_new(0.0f, 0.0f, 3.0f);
            game->cam_yaw = -90.0f;
            game->cam_pitch = 0.0f;
            game->cam_yaw_target = game->cam_yaw;
            game->cam_pitch_target = game->cam_pitch;
	}

    /*
	printf("x: %f, y: %f, yaw: %f, pitch: %f\n",
    	game->cam_pos_target.x,
    	game->cam_pos_target.y,
    	game->cam_yaw,
    	game->cam_pitch);
	*/

	// Populate render group
	render_group->clear_color = v3_new(0.8f, 0.8f, 0.8f);
	
	// Update camera target from cam_yaw/pitch
	render_group->cam_pos = game->cam_pos;
	render_group->cam_target = v3_add(game->cam_pos, cam_forward);
	
	render_group->lines_len = MAX_NETWORK_LINES;
	memcpy(&render_group->lines, &game->lines, MAX_NETWORK_LINES * sizeof(struct v3_line));

	game->pulse += dt * 5;
	if(game->pulse > 8) {
    	game->pulse = game->pulse - 8;
	}

	for(uint32_t i = 0; i < MAX_NETWORK_LINES; i++) {
		render_group->line_colors[i] = (struct v3_line){v3_new(0.1, 0.1, 0.1), v3_new(0.1, 0.1, 0.1)};
	}

	if(game->pulse > 0 && game->pulse < 1) {
    	float rg_a = lerp(1.0, 0.1, game->pulse);
    	float rg_b = lerp(0.1, 1.0, game->pulse);
		render_group->line_colors[0] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(rg_b, rg_b, 0.1)};

    	rg_a = lerp(0.1, 1.0, game->pulse);
		render_group->line_colors[1] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(0.1, 0.1, 0.1)};
	}
	if(game->pulse > 1 && game->pulse < 2) {
    	float rg_b = lerp(1.0, 0.1, game->pulse - 1);
		render_group->line_colors[0] = (struct v3_line){v3_new(0.1, 0.1, 0.1), v3_new(rg_b, rg_b, 0.1)};

    	float rg_a = lerp(1.0, 0.1, game->pulse - 1);
    	rg_b = lerp(0.1, 1.0, game->pulse - 1);
		render_group->line_colors[1] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(rg_b, rg_b, 0.1)};

    	rg_a = lerp(0.1, 1.0, game->pulse - 1);
		render_group->line_colors[2] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(0.1, 0.1, 0.1)};
		render_group->line_colors[3] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(0.1, 0.1, 0.1)};
	}
	if(game->pulse > 2 && game->pulse < 3) {
    	float rg_b = lerp(1.0, 0.1, game->pulse - 2);
		render_group->line_colors[1] = (struct v3_line){v3_new(0.1, 0.1, 0.1), v3_new(rg_b, rg_b, 0.1)};

    	float rg_a = lerp(1.0, 0.1, game->pulse - 2);
    	rg_b = lerp(0.1, 1.0, game->pulse - 2);
		render_group->line_colors[2] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(rg_b, rg_b, 0.1)};
		render_group->line_colors[3] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(rg_b, rg_b, 0.1)};
	}
	if(game->pulse > 3 && game->pulse < 4) {
    	float rg_b = lerp(1.0, 0.1, game->pulse - 3);
		render_group->line_colors[2] = (struct v3_line){v3_new(0.1, 0.1, 0.1), v3_new(rg_b, rg_b, 0.1)};
		render_group->line_colors[3] = (struct v3_line){v3_new(0.1, 0.1, 0.1), v3_new(rg_b, rg_b, 0.1)};
	}
	if(game->pulse > 7 && game->pulse < 8) {
    	float rg_a = lerp(0.1, 1.0, game->pulse - 7);
		render_group->line_colors[0] = (struct v3_line){v3_new(rg_a, rg_a, 0.1), v3_new(0.1, 0.1, 0.1)};
	}
}
