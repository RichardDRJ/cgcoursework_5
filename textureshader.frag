#version 150

in vertexData
{
	vec3 pos;
	vec3 normal;
	vec4 colour;
	vec2 tex_coords;
}vertex;

out vec4 fragColor;

uniform sampler2D texMap;

void main()
{
    vec4 diffuseTexel = texture( texMap, vertex.tex_coords );

    fragColor = diffuseTexel;
//    fragColor = vec4(0, vertex.tex_coords ,1);
//    fragColor = vec4(1,1,1,1);
}