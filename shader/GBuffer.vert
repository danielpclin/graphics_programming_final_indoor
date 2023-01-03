#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord0;
// layout (location = 3) in vec3 tangent;

out VS_OUT
{ 
    vec3 ws_coords; // world-space coord
    vec3 normal; 
    vec3 tangent; 
    vec2 texcoord0; 
} vs_out;

uniform mat4 projection; 
uniform mat4 view; 
uniform mat4 model;

void main(void)
{
    mat4 mv_matrix = view * model; 
    gl_Position = projection * mv_matrix * vec4(position, 1.0); 
    vs_out.ws_coords = (model * vec4(position, 1.0)).xyz;
    vs_out.normal = mat3(transpose(inverse(model))) * normal; 
    // vs_out.normal = normal; 
    // vs_out.tangent = tangent;
    vs_out.texcoord0 = texcoord0;
}
