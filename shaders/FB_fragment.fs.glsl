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
        float offset = 0.005;
        vec2 leftUV = fs_in.texcoord + vec2(-offset, 0.0);
        vec2 rightUV = fs_in.texcoord + vec2(offset, 0.0);

        vec3 leftColor = texture(tex, leftUV).rgb;
        vec3 rightColor = texture(tex, rightUV).rgb;

        float Color_R = leftColor.r * 0.299 + leftColor.g * 0.587 + leftColor.b * 0.114;
        float Color_G = rightColor.g;
        float Color_B = rightColor.b;

        color = vec4(Color_R, Color_G, Color_B, 1.0);
        // END ANSWER
}
