#version 410 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexture;
layout (location = 3) in vec3 inTangent;

out vec3 position;
out vec3 normal;
out vec2 textureCoordinate;
out vec3 shadowPosition;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 depthMVP;

void main(void)
{
    position = vec3(model * vec4(inPosition, 1.0));
    normal = mat3(transpose(inverse(model))) * inNormal;
    vec3 T = normalize(vec3(model * vec4(inTangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(inNormal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    TBN = mat3(T, B, N);
    textureCoordinate = inTexture;

    shadowPosition = vec3(depthMVP * vec4(inPosition, 1.0));

    gl_Position = projection * view * model * vec4(inPosition, 1.0);
}
