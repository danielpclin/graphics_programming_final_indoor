#version 410 core

layout (location = 0) in vec3 aPos;

out VertexData
{
	vec2 texcoord;
} vertexData;

void main()
{
	vertexData.texcoord = aPos.xy * 0.5 + 0.5;
	gl_Position = vec4(aPos, 1.0);
}