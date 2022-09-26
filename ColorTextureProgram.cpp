#include "ColorTextureProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Load< ColorTextureProgram > color_texture_program(LoadTagEarly);

ColorTextureProgram::ColorTextureProgram() {
	// Shader code from https://learnopengl.com/In-Practice/Text-Rendering
	program = gl_compile_program(
		//vertex shader:
		"#version 330 core\n"
		"layout (location = 0) in vec4 vertex;\n"
		"out vec2 TexCoords;\n"
		"uniform mat4 projection;\n"
		"void main() {\n"
		"	gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
		"	TexCoords = vertex.zw;\n"
		"}\n"
		,
		//fragment shader:
		"#version 330 core\n"
		"in vec2 TexCoords;\n"
		"out vec4 color;\n"
		"uniform sampler2D text;\n"
		"uniform vec3 textColor;\n"
		"void main() {\n"
		"	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
		"	color = vec4(textColor, 1.0) * sampled;\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	vertex_vec4 = glGetAttribLocation(program, "Position");
	TexCoords_vec2 = glGetAttribLocation(program, "TexCoords");

	//look up the locations of uniforms:
	projection_mat4 = glGetUniformLocation(program, "projection");
	textColor_vec3 = glGetUniformLocation(program, "textColor");
	GLuint text_sampler2D = glGetUniformLocation(program, "text");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(text_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

ColorTextureProgram::~ColorTextureProgram() {
	if (program != 0) {
		glDeleteProgram(program);
		program = 0;
	}
}

