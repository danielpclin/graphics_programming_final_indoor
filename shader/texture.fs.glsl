#version 410 core

in vec3 position;
in vec3 normal;
in vec2 textureCoordinate;
in vec3 shadowPosition;

layout (location = 0) out vec4 color;

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

uniform Config config;

void main(void)
{
    vec4 textureColor = texture(textureMap, textureCoordinate).rgba;
    vec3 normalizedNormal = normalize(normal);

    if (textureColor.a < 0.5)
        discard;

    if (!hasTexture)
        textureColor = vec4(1.0);


    // ambient
    vec3 ambient = material.ambient * textureColor.rgb;

    // diffuse
    vec3 lightDirection = normalize(light.position - position);
    vec3 diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * textureColor.rgb * material.diffuse;

    // specular
    vec3 viewDirection = normalize(cameraPosition - position);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    vec3 specular = pow(max(dot(normalizedNormal, halfwayDirection), 0.0), material.shininess) * material.specular;

    // directional light shadow
    float bias = max(0.05 * (1.0 - dot(normalizedNormal, lightDirection)), 0.005);
    bias = 0.005;

    float visibility = 1.0;
    if (texture(shadowMap, shadowPosition.xy).x < shadowPosition.z - bias){
        visibility = 0.5;
    }

    color = vec4(textureColor.rgb * material.diffuse, 1.0);

    if (config.blinnPhong) {
        color = vec4((ambient * light.ambient + diffuse * light.diffuse + specular * light.specular), 1.0);
    }

    if (config.directionalLightShadow) {
        color = vec4(visibility * (ambient * light.ambient + diffuse * light.diffuse + specular * light.specular), 1.0);
    }

    if (config.bloom) {

    }

    if (config.deferredShading) {

    }

    if (config.normalMapping) {

    }

}
