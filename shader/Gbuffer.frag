#version 430 core
layout (location = 0) out vec4 color0; //Diffuse map
layout (location = 1) out vec4 color1; //Normal map
layout (location = 2) out vec4 color2; //Coordinate
layout (location = 3) out vec4 color3; //Ambient
layout (location = 4) out vec4 color4; //Specular
in VS_OUT
{ 
    vec3 ws_coords; 
    vec3 normal; 
    vec3 tangent; 
    vec2 texcoord0; 
    mat3 TBN;
} fs_in;

uniform bool hasTexture;
uniform sampler2D tex_diffuse;
uniform bool hasNormalMap;
uniform sampler2D NormalMap;
// layout (binding = 1) uniform sampler2D tex_normal_map;      

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

uniform Material material;
void main(void)
{ 

    vec3 nm = fs_in.normal;
    vec4 outvec2 = vec4(0);
    outvec2.xyz = fs_in.ws_coords;
    outvec2.w = 60.0; // specular_power
    if (hasTexture)
        color0 = vec4(texture(tex_diffuse, fs_in.texcoord0).rgb, 1.0); // diffuse
    else
        color0 = vec4(material.diffuse, 1.0);
    if (hasNormalMap)
    {
        vec3 normalizedNormal;
        normalizedNormal = texture(NormalMap, fs_in.texcoord0).xyz;
        normalizedNormal = normalizedNormal * 2.0 - vec3(1.0);   
        normalizedNormal = normalize(fs_in.TBN * normalizedNormal);
        nm = normalizedNormal;
    }
    color1 = vec4(nm,1.0); // normal
    color3 = vec4(material.ambient, 1.0);
    color4 = vec4(material.specular, 1.0);
    color2 = outvec2; // world coord + specular power
}
