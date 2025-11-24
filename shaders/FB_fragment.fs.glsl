#version 410 core

uniform sampler2D tex;

out vec4 color;

in VS_OUT
{
	vec2 texcoord;
} fs_in;

void main()
{
	// #TODO 9: Do Red-Blue Stereo here 
	// BEGIN ANSWER
	color = vec4(0.0);
	// END ANSWER
}