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
} fs_in;

uniform bool hasTexture;
uniform sampler2D tex_diffuse;
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
    // vec3 N = normalize(fs_in.normal);
    // vec3 T = normalize(fs_in.tangent);
    // vec3 B = cross(N, T);
    // mat3 TBN = mat3(T, B, N);
    // vec3 nm = texture(tex_normal_map,fs_in.texcoord0).xyz * 2.0 - 1.0;
    // nm = TBN * normalize(nm); 

    vec3 nm = fs_in.normal;
    vec4 outvec2 = vec4(0);
    outvec2.xyz = fs_in.ws_coords;
    outvec2.w = 60.0; // specular_power
    if (hasTexture) {
        vec4 temp = texture(tex_diffuse, fs_in.texcoord0);
        if (temp.a < 0.5) {
            discard;
        }
        color0 = vec4(temp.rgb, 1.0); // diffuse
    } else {
        color0 = vec4(material.diffuse, 1.0);
    }
    color1 = vec4(nm,1.0); // normal
    color3 = vec4(material.ambient, 1.0);
    color4 = vec4(material.specular, 1.0);
    color2 = outvec2; // world coord + specular power
}
