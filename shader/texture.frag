#version 410 core

in vec3 position;
in vec3 normal;
in vec2 textureCoordinate;
in vec3 shadowPosition;

layout (location = 0) out vec4 color;
//*----- Bloom Effect Layout Begin ----- */
layout (location = 1) out vec4 BloomEffect_BrightColor;
//*----- Bloom Effect Layout End ----- */

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Config {
    bool blinnPhong;
    bool directionalLightShadow;
    bool bloom;
    bool deferredShading;
    bool normalMapping;
};

uniform bool hasTexture;
uniform sampler2D textureMap;
uniform sampler2D shadowMap;
uniform vec3 cameraPosition;
uniform Light light;
uniform Material material;

//*----- Bloom Effect Uniforms Begin ----- */
uniform vec3 emissive_sphere_position;
uniform bool isLightObject;
//*----- Bloom Effect Uniforms End ----- */

uniform Config config;

float random(vec4 seed4) {
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

void main(void)
{
    vec4 textureColor = texture(textureMap, textureCoordinate).rgba;
    vec3 normalizedNormal = normalize(normal);
    vec3 lightDirection = normalize(light.position - position);
    vec3 viewDirection = normalize(cameraPosition - position);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    if (textureColor.a < 0.5)
        discard;

    if (!hasTexture)
        textureColor = vec4(1.0);

    // ambient
    vec3 ambient = material.diffuse;

    // diffuse
    vec3 diffuse = material.diffuse;

    // specular
    vec3 specular = material.diffuse;

    if (config.blinnPhong) {
        ambient = material.ambient * textureColor.rgb;
        diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * textureColor.rgb * material.diffuse;
        specular = pow(max(dot(normalizedNormal, halfwayDirection), 0.0), material.shininess) * material.specular;
    }

    // directional light shadow
    float bias = max(0.06 * (1.0 - dot(normalizedNormal, lightDirection)), 0.01);

    float shadow = 0.0;
    vec2 texelSize = 0.4 / textureSize(shadowMap, 0);
    for(int x = -5; x <= 5; ++x) {
        for(int y = -5; y <= 5; ++y) {
            float pcfDepth = texture(shadowMap, shadowPosition.xy + vec2(x, y) * texelSize).x;
            shadow += shadowPosition.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 121.0;
    shadow = 1 - shadow;

    ambient = ambient * light.ambient;
    diffuse = diffuse * light.diffuse;
    specular = specular * light.specular;
    color = vec4((ambient + diffuse + specular), 1.0);

    if (config.directionalLightShadow) {
        color = vec4((ambient + shadow * (diffuse + specular)), 1.0);
    }

    if (config.bloom) {
        //*----- Bloom Effect Begin ----- */
        float emissive_sphere_distance = length(emissive_sphere_position - position);
        float attenuation = 1.0 / (1.0 + 0.7 * emissive_sphere_distance + 0.14 * (emissive_sphere_distance * emissive_sphere_distance)); 
        //ambient  *= attenuation;
        //diffuse  *= attenuation;
        //specular *= attenuation;
        color = vec4(color.xyz*(1+attenuation), 1.0);
        if (isLightObject == true) color = vec4(vec3(2.0), 1.0);

        float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
        if(brightness > 1.0)
            BloomEffect_BrightColor = vec4(color.rgb, 1.0);
        else
            BloomEffect_BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        //*----- Bloom Effect End -----*/
    }

    if (config.deferredShading) {

    }

    if (config.normalMapping) {

    }

}
