#version 410

layout(location = 0) out vec4 fragColor;

uniform bool ubflag;
uniform sampler2D tex;

in VertexData
{
    vec3 N; // eye space normal
    vec3 L; // eye space light vector
    vec3 H; // eye space halfway vector
    vec2 texcoord;
} vertexData;

void main()
{
	// #TODO 4: Modify fragment shader
    // BEGIN ANSWER
    fragColor = texture(tex, vertexData.texcoord);
    // END ANSWER
}