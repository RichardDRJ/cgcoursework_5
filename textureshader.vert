#version 150

in vec3 in_position;
in vec2 in_texCoord;

uniform vec4 in_colour;
uniform mat4 projection_matrix;
uniform mat4 model_view_matrix;

out vertexData
{
	vec3 pos;
	vec3 normal;
	vec4 colour;
	vec2 tex_coords;
}vertex;

void main()
{
	vertex.colour = in_colour;
	vertex.tex_coords = in_texCoord;
	vertex.pos = in_position;

    gl_Position = vec4(in_position, 1.0);
}