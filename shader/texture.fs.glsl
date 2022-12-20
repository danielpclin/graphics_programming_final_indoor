#version 410 core

in vec3 position;
in vec3 normal;
in vec2 textureCoordinate;

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

    color = vec4(textureColor.rgb * material.diffuse, 1.0);

    if (config.blinnPhong) {
        // ambient
        vec3 ambient = material.ambient * textureColor.rgb;

        // diffuse
        vec3 lightDirection = normalize(light.position - position);
        vec3 diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * textureColor.rgb * material.diffuse;

        // specular
        vec3 viewDirection = normalize(cameraPosition - position);
        vec3 halfwayDirection = normalize(lightDirection + viewDirection);
        vec3 specular = pow(max(dot(normalizedNormal, halfwayDirection), 0.0), material.shininess) * material.specular;
        color = vec4((ambient * light.ambient + diffuse * light.diffuse + specular * light.specular), 1.0);
    }

    if (config.directionalLightShadow) {

    }

    if (config.bloom) {

    }

    if (config.deferredShading) {

    }

    if (config.normalMapping) {

    }

}
