#version 410 core

in vec3 position;
in vec3 normal;
in vec2 textureCoordinate;
in vec3 shadowPosition;
in mat3 TBN;

layout (location = 0) out vec4 color;
//*----- Bloom Effect Layout Begin ----- */
layout (location = 1) out vec4 BloomEffect_BrightColor;
//*----- Bloom Effect Layout End ----- */

struct DirectionalLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
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
    bool NPR;
    bool SSAO;
};

struct View {
    int width;
    int height;
};

uniform bool hasTexture;
uniform sampler2D textureMap;
uniform bool hasNormalMap;
uniform sampler2D NormalMap;
uniform sampler2D shadowMap;
uniform sampler2D SSAO_Map;
uniform vec3 cameraPosition;
uniform DirectionalLight directionalLight;
uniform PointLight pointLight; // TODO add point light
uniform Material material;
uniform View view;

// /*----- Bloom Effect Uniforms Begin ----- */
uniform vec3 emissive_sphere_position; // TODO replace with pointLight.position
uniform bool isLightObject;
// /*----- Bloom Effect Uniforms End ----- */

uniform Config config;

float random(vec4 seed4) {
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(dot_product) * 43758.5453);
}

void main(void)
{
    vec4 textureColor = texture(textureMap, textureCoordinate).rgba;
    vec3 normalizedNormal = normalize(normal);
    if (hasNormalMap) {
        normalizedNormal = texture(NormalMap, textureCoordinate).rgb;
        normalizedNormal = normalizedNormal * 2.0 - 1.0;
        normalizedNormal = normalize(TBN * normalizedNormal);
    }
    vec3 lightDirection = normalize(directionalLight.position - position);
    vec3 viewDirection = normalize(cameraPosition - position);
    vec3 directionalLight_LightDirection = normalize(directionalLight.position - position);
    vec3 directionalLight_HalfwayDirection = normalize(directionalLight_LightDirection + viewDirection);

    if (textureColor.a < 0.5)
        discard;

    if (!hasTexture)
        textureColor = vec4(1.0);

    // ambient diffuse specular
    vec3 ambient = material.ambient * textureColor.rgb * directionalLight.ambient;
    vec3 diffuse = textureColor.rgb * material.diffuse;
    vec3 specular = vec3(0.0);
    color = vec4(diffuse, 1.0);

    if (config.SSAO) {
        vec2 p = vec2(gl_FragCoord.x / view.width, gl_FragCoord.y / view.height);
        ambient *= vec4(vec3(texture(SSAO_Map, p)), 1.0).r;
    }

    if (config.blinnPhong) {
        diffuse = max(dot(normalizedNormal, directionalLight_LightDirection), 0.0) * textureColor.rgb * material.diffuse * directionalLight.diffuse;
        specular = pow(max(dot(normalizedNormal, directionalLight_HalfwayDirection), 0.0), material.shininess) * material.specular * directionalLight.specular;
        color = vec4((ambient + diffuse + specular), 1.0);
    }

    // directional light shadow
    float bias = max(0.06 * (1.0 - dot(normalizedNormal, directionalLight_LightDirection)), 0.01);

    float shadow = 0.0;
    vec2 texelSize = 0.4 / textureSize(shadowMap, 0);
    for(int x = -5; x <= 5; ++x) {
        for(int y = -5; y <= 5; ++y) {
            float pcfDepth = texture(shadowMap, shadowPosition.xy + vec2(x, y) * texelSize).x;
            shadow += shadowPosition.z - bias > pcfDepth ? 0.8 : 0.0;
        }
    }
    shadow /= 121.0;
    shadow = 1 - shadow;

    if (config.directionalLightShadow) {
        color = vec4((ambient + shadow * (diffuse + specular)), 1.0);
    }

    if (config.bloom) {
        // /*----- Bloom Effect Begin ----- */
        vec3 Bloom_lightDir = normalize(emissive_sphere_position - position);
        // diffuse shading
        float Bloom_diff = max(dot(normalizedNormal, Bloom_lightDir), 0.0);
        // specular shading
        //vec3 Bloom_reflectDir = reflect(-Bloom_lightDir, normalizedNormal);
        vec3 Bloom_halfwayDir = normalize(Bloom_lightDir + viewDirection);
        float Bloom_spec = pow(max(dot(viewDirection, Bloom_halfwayDir), 0.0), material.shininess);
        // attenuation
        float emissive_sphere_distance = length(emissive_sphere_position - position);
        float attenuation = 1.0 / (1.0 + 0.7 * emissive_sphere_distance + 0.14 * (emissive_sphere_distance * emissive_sphere_distance)); 
        // combine results
        vec3 Bloom_ambient  = directionalLight.ambient * material.ambient;
        vec3 Bloom_diffuse  = directionalLight.diffuse * material.diffuse * Bloom_diff;
        vec3 Bloom_specular = directionalLight.specular * Bloom_spec * material.specular;
        Bloom_ambient  *= attenuation;
        Bloom_diffuse  *= attenuation;
        Bloom_specular *= attenuation;
        vec3 Bloom_color = Bloom_ambient + Bloom_diffuse + Bloom_specular;
        color = vec4(color.xyz+Bloom_color, 1.0);
        if (isLightObject == true) color = vec4(vec3(2.0), 1.0);
        
        float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
        if(brightness > 1.0)
            BloomEffect_BrightColor = vec4(color.rgb, 1.0);
        else
            BloomEffect_BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
        // /*----- Bloom Effect End -----*/
    }

    if (config.deferredShading) {

    }

    if (config.normalMapping) {

    }

}
