const char* vertex_shader_src =
"#version 330 core\n"
"layout (location = 0) in vec3 a_pos;\n"
"layout (location = 1) in vec3 b_pos;\n"
"layout (location = 2) in float orientation;\n"
"layout (location = 3) in float startedness;\n"

"uniform mat4 projection;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"

"uniform vec3 a_color;\n"
"uniform vec3 b_color;\n"

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

	"float width = 0.004;\n"
	"normal *= width / 2.0;\n"

	"vec4 offset = vec4(normal * orientation * a_proj.z, 0.0, 0.0);\n"
	"gl_Position = a_proj + offset;\n"

	"vec3 base_color = mix(a_color, b_color, startedness);\n"
	"my_color = mix(base_color, vec3(0.72, 0.72, 0.72), clamp(a_proj.z / 4, 0, 0.9));\n"
"}\0";

const char* frag_shader_src=
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 my_color;\n"
"void main()\n"
"{\n"
"	FragColor = vec4(my_color, 1.0f);\n"
"}\0";
