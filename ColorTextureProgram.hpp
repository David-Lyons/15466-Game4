#pragma once

#include "GL.hpp"
#include "Load.hpp"

//Shader program that draws transformed, vertices tinted with vertex colors:
struct ColorTextureProgram {
	ColorTextureProgram();
	~ColorTextureProgram();

	GLuint program = 0;
	//Attribute (per-vertex variable) locations:
	GLuint vertex_vec4 = -1U;
	GLuint TexCoords_vec2 = -1U;
	//Uniform (per-invocation variable) locations:
	GLuint projection_mat4 = -1U;
	GLuint textColor_vec3 = -1U;
	//Textures:
	//TEXTURE0 - texture that is accessed by TexCoord
};

extern Load< ColorTextureProgram > color_texture_program;
