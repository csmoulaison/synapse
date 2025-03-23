// TODO - Our own linalg stuff?
// TODO - Don't hardcode colors, have that all come from the render_group.
void gl_render(struct x11_context* x11, struct render_group* group) 
{
	// See the following link for an analagous scenario:
	// https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/6.3.coordinate_systems_multiple/coordinate_systems_multiple.cpp
	glClearColor(
    	group->clear_color.r, 
    	group->clear_color.g, 
    	group->clear_color.b, 
    	1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// TODO - experiment calling this after projection to see if that can be
	// moved out of this loop.
	glUseProgram(x11->gl.shader_program_lines);
	glBindVertexArray(x11->gl.vertex_array_object);

	// Maybe do outside of this loop?
	mat4 projection;
	// TODO - second param should be width/height.
	glm_perspective(glm_rad(75.0f), (float)x11->winw/ (float)x11->winh, 0.05f, 100.0f, projection);
	uint32_t projection_loc = glGetUniformLocation(x11->gl.shader_program_lines, "projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, projection[0]);

	// View matrix
	mat4 view = GLM_MAT4_IDENTITY_INIT;
	struct v3 up = v3_new(0.0f, 1.0f, 0.0f);
	m4_lookat(
    	group->cam_pos,
    	group->cam_target,
    	up,
    	view);
	uint32_t view_loc = glGetUniformLocation(x11->gl.shader_program_lines, "view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, view[0]);

	// Draw lines
	for(uint32_t i = 0; i < group->lines_len; i++) {
    	struct v3 line_a = group->lines[i].a;
    	struct v3 line_b = group->lines[i].b;

    	mat4 model = GLM_MAT4_IDENTITY_INIT;
    	glm_translate(model, (vec3){line_a.x, line_a.y, line_a.z});
    	glm_scale(model, (vec3){line_b.x - line_a.x, line_b.y - line_a.y, line_b.z - line_a.z});

    	uint32_t model_loc = glGetUniformLocation(x11->gl.shader_program_lines, "model");
    	glUniformMatrix4fv(model_loc, 1, GL_FALSE, model[0]);

		struct v3 a_color = group->line_colors[i].a;
    	uint32_t a_color_loc = glGetUniformLocation(x11->gl.shader_program_lines, "a_color");
    	glUniform3f(a_color_loc, a_color.r, a_color.g, a_color.b);

		struct v3 b_color = group->line_colors[i].b;
    	uint32_t b_color_loc = glGetUniformLocation(x11->gl.shader_program_lines, "b_color");
    	glUniform3f(b_color_loc, b_color.r, b_color.g, b_color.b);

    	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}

	glXSwapBuffers(x11->display, x11->window);
}
