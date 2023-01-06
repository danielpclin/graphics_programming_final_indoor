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
    bool areaLight;
};

struct View {
    int width;
    int height;
};

struct AreaLight {
    float intensity;
    vec3 color;
    vec3 points[4];
    bool twoSided;
};

uniform bool hasTexture;
uniform sampler2D textureMap;
uniform bool hasNormalMap;
uniform sampler2D NormalMap;
uniform sampler2D shadowMap;
uniform float farPlane;
uniform samplerCube pointShadowMap;
uniform sampler2D SSAO_Map;
uniform vec3 cameraPosition;
uniform DirectionalLight directionalLight;
uniform PointLight pointLight; // TODO add point light
uniform Material material;
uniform View view;

//*----- Bloom Effect Uniforms Begin ----- */
uniform vec3 emissive_sphere_position; // TODO replace with pointLight.position
uniform bool isLightObject;
//*----- Bloom Effect Uniforms End ----- */

//*----- NPR Variables Begin ----- */
float nDotL;
//*----- NPR Variables End ----- */

// Area_Light Uniforms Begin
uniform AreaLight areaLight;
uniform mat4 areaLightModel;
uniform vec3 viewPosition;
uniform sampler2D LTC1; // for inverse M
uniform sampler2D LTC2; // GGX norm, fresnel, 0(unused), sphere

const float LUT_SIZE  = 64.0; // ltc_texture size
const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
const float LUT_BIAS  = 0.5/LUT_SIZE;
// Area_Light Uniforms End

uniform Config config;

// Vector form without project to the plane (dot with the normal)
// Use for proxy sphere clipping
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    // Using built-in acos() function will result flaws
    // Using fitting result for calculating acos()
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

    return cross(v1, v2)*theta_sintheta;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
    return IntegrateEdgeVec(v1, v2).z;
}

// P is fragPos in world space (LTC distribution)
vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));

    // polygon (allocate 4 vertices for clipping)
    vec3 L[4];
    // transform polygon from LTC back to origin Do (cosine weighted)
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    // use tabulated horizon-clipped sphere
    // check if the shading point is behind the light
    vec3 dir = points[0] - P; // LTC space
    vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
    bool behind = (dot(dir, lightNormal) < 0.0);

    // cos weighted space
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    // integrate
    vec3 vsum = vec3(0.0);
    vsum += IntegrateEdgeVec(L[0], L[1]);
    vsum += IntegrateEdgeVec(L[1], L[2]);
    vsum += IntegrateEdgeVec(L[2], L[3]);
    vsum += IntegrateEdgeVec(L[3], L[0]);

    // form factor of the polygon in direction vsum
    float len = length(vsum);

    float z = vsum.z/len;
    if (behind)
        z = -z;

    vec2 uv = vec2(z*0.5f + 0.5f, len); // range [0, 1]
    uv = uv*LUT_SCALE + LUT_BIAS;

    // Fetch the form factor for horizon clipping
    float scale = texture(LTC2, uv).w;

    float sum = len*scale;
    if (!behind && !twoSided)
        sum = 0.0;

    // Outgoing radiance (solid angle) for the entire polygon
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}

// PBR-maps for roughness (and metallic) are usually stored in non-linear
// color space (sRGB), so we use these functions to convert into linear RGB.
vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float gamma = 2.2;
vec3 ToLinear(vec3 v) { return PowVec3(v, gamma); }
vec3 ToSRGB(vec3 v)   { return PowVec3(v, 1.0/gamma); }

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

    if (config.NPR) {
        nDotL = dot(normalizedNormal, lightDirection);
        diffuse = diffuse * floor(nDotL * 3) / 3;
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
        if (config.NPR) {
            nDotL = dot(normalizedNormal, Bloom_lightDir);
            Bloom_diffuse = Bloom_diffuse * floor(nDotL * 3) / 3;
        }
        Bloom_specular *= attenuation;

        // point light shadow
        vec3 fragToLight = position - emissive_sphere_position;
        float closestDepth = texture(pointShadowMap, fragToLight).r;
        closestDepth *= farPlane;
        float currentDepth = length(fragToLight);
        bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
        shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;  
        shadow = (1.0 - shadow);
        
        vec3 Bloom_color = Bloom_ambient + Bloom_diffuse * shadow + Bloom_specular * shadow;
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

    if (config.areaLight) {
        specular = ToLinear(material.specular);
        vec3 result = vec3(0.0f);

        vec3 N = normalize(normal);
        vec3 V = normalize(viewPosition - position);
        vec3 P = position;

        float dotNV = clamp(dot(N, V), 0.0f, 1.0f);

        vec2 uv = vec2(0.5f, sqrt(1.0f - dotNV));
        uv = uv*LUT_SCALE + LUT_BIAS;

        // get 4 parameters for inverse_M
        vec4 t1 = texture(LTC1, uv);

        // Get 2 parameters for Fresnel calculation
        vec4 t2 = texture(LTC2, uv);

        mat3 Minv = mat3(
            vec3(t1.x, 0, t1.y),
            vec3(  0,  1,    0),
            vec3(t1.z, 0, t1.w)
        );

        // translate light source for testing
        vec4 translatedPoints_4[4];
        translatedPoints_4[0] = areaLightModel * vec4(areaLight.points[0],1.0);
        translatedPoints_4[1] = areaLightModel * vec4(areaLight.points[1],1.0);
        translatedPoints_4[2] = areaLightModel * vec4(areaLight.points[2],1.0);
        translatedPoints_4[3] = areaLightModel * vec4(areaLight.points[3],1.0);

        vec3 translatedPoints_3[4];
        translatedPoints_3[0] = vec3(translatedPoints_4[0].x,translatedPoints_4[0].y,translatedPoints_4[0].z);
        translatedPoints_3[1] = vec3(translatedPoints_4[1].x,translatedPoints_4[1].y,translatedPoints_4[1].z);
        translatedPoints_3[2] = vec3(translatedPoints_4[2].x,translatedPoints_4[2].y,translatedPoints_4[2].z);
        translatedPoints_3[3] = vec3(translatedPoints_4[3].x,translatedPoints_4[3].y,translatedPoints_4[3].z);


        // Evaluate LTC shading
        vec3 LTC_diffuse = LTC_Evaluate(N, V, P, mat3(1), translatedPoints_3, areaLight.twoSided);
        vec3 LTC_specular = LTC_Evaluate(N, V, P, Minv, translatedPoints_3, areaLight.twoSided);

        // GGX BRDF shadowing and Fresnel
        // t2.x: shadowedF90 (F90 normally it should be 1.0)
        // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
        LTC_specular *= specular*t2.x + (1.0f - specular) * t2.y;

        result = areaLight.color * areaLight.intensity * (LTC_specular + diffuse * LTC_diffuse);
        result = ToSRGB(result);
        color += vec4(result, 1.0f);
    }
}
