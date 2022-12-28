#version 410
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D colorTexture;

void main()
{
	vec3 color = vec3(texture(colorTexture, TexCoords));

    FragColor = vec4(color, 1.0);
//    FragColor = vec4(color, 1.0);
}
