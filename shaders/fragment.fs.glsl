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
    vec3 ia = vec3(0.3);
    vec3 id = ubflag ? vec3(0.3, 0.3, 0.9) : vec3(0.5);
    vec3 is = ubflag ? vec3(0.5, 0.5, 0.9) : vec3(1.0);

    vec3 texColor = texture(tex, vertexData.texcoord).rgb;
    vec3 ka = texColor;
    vec3 kd = texColor;
    vec3 ks = vec3(1.0);

    vec3 N = normalize(vertexData.N);
    vec3 L = normalize(vertexData.L);
    vec3 H = normalize(vertexData.H);

    float diff = max(dot(N, L), 0.0);
    float spec = diff > 0.0 ? pow(max(dot(N, H), 0.0), 100.0) : 0.0;

    vec3 color = ia * ka + id * kd * diff + is * ks * spec;
    fragColor = vec4(color, 1.0);
    // END ANSWER
}
