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

uniform sampler2D textureMap;
uniform vec3 cameraPosition;
uniform Light light;
uniform Material material;

void main(void)
{
    vec3 textureColor = texture(textureMap, textureCoordinate).rgb;

    vec3 normalizedNormal = normalize(normal);

    // ambient
    vec3 ambient = material.ambient * textureColor;

    // diffuse
    vec3 lightDirection = normalize(light.position - position);
    vec3 diffuse = max(dot(normalizedNormal, lightDirection), 0.0) * textureColor * material.diffuse;

    // specular
    vec3 viewDirection = normalize(cameraPosition - position);
    vec3 reflectDirection = reflect(-lightDirection, normalizedNormal);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    vec3 specular = pow(max(dot(normalizedNormal, halfwayDirection), 0.0), material.shininess * 4) * material.specular;

    color = vec4((ambient * light.ambient + diffuse * light.diffuse + specular * light.specular), 1.0);
}