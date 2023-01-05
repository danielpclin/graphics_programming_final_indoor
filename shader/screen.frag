#version 410
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

struct Config {
    bool blinnPhong;
    bool directionalLightShadow;
    bool bloom;
    bool deferredShading;
    bool normalMapping;
    bool NPR;
};
uniform Config config;

uniform sampler2D colorTexture;
uniform sampler2D BloomEffect_HDR_Texture;
uniform sampler2D BloomEffect_Blur_Texture;

// /*----- Deferred Shading -----*/
//g buffers
uniform sampler2D gtex[5];

uniform int gbufferidx;

void main()
{
	vec3 color = vec3(texture(colorTexture, TexCoords));

    FragColor = vec4(color, 1.0);

    // /*----- Bloom Effect Begin ----- */
    if (config.bloom && config.NPR==false) {
        //FragColor = vec4(BloomEffect_HDR_Color, 1.0);
        const float gamma = 2.2;
        const float exposure = 0.01;
        vec3 hdrColor = vec3(texture(BloomEffect_HDR_Texture, TexCoords));
        vec3 blurColor = vec3(texture(BloomEffect_Blur_Texture, TexCoords));
        hdrColor += blurColor; // additive blending
        // tone mapping
        vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
        // also gamma correct while we're at it       
        result = pow(result, vec3(1.0 / gamma));
        result += blurColor;
        FragColor = vec4(color + result, 1.0);
    }
    // /*----- Bloom Effect End -----*/

    // /*----- NPR Edge Detection Begin -----*/
    if (config.NPR) {
        vec2 tex_offset = 1.0 / textureSize(colorTexture, 0); // gets size of single texel
        float Gx = 0;
        float Gy = 0;

        Gx += length(texture(colorTexture, TexCoords - vec2(tex_offset.x, tex_offset.y)).rgb) * 1;
        Gx += length(texture(colorTexture, TexCoords - vec2(tex_offset.x, 0.0)).rgb) * 2;
        Gx += length(texture(colorTexture, TexCoords - vec2(tex_offset.x, -tex_offset.y)).rgb) * 1;
        Gx += length(texture(colorTexture, TexCoords + vec2(tex_offset.x, tex_offset.y)).rgb) * -1;
        Gx += length(texture(colorTexture, TexCoords + vec2(tex_offset.x, 0.0)).rgb) * -2;
        Gx += length(texture(colorTexture, TexCoords + vec2(tex_offset.x, -tex_offset.y)).rgb) * -1;

        Gy += length(texture(colorTexture, TexCoords - vec2(tex_offset.x, tex_offset.y)).rgb) * 1;
        Gy += length(texture(colorTexture, TexCoords - vec2(0.0, tex_offset.y)).rgb) * 2;
        Gy += length(texture(colorTexture, TexCoords - vec2(-tex_offset.x, tex_offset.y)).rgb) * 1;
        Gy += length(texture(colorTexture, TexCoords + vec2(tex_offset.x, tex_offset.y)).rgb) * -1;
        Gy += length(texture(colorTexture, TexCoords + vec2(0.0, tex_offset.y)).rgb) * -2;
        Gy += length(texture(colorTexture, TexCoords + vec2(-tex_offset.x, -tex_offset.y)).rgb) * -1;
    
        float G = sqrt(Gx*Gx+Gy*Gy);
        if (G > length(color) * 0.5) FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    
    }
    // /*----- NPR Edge Detection End -----*/

    if (config.deferredShading) {
        vec3 gcolor = texture(gtex[gbufferidx], TexCoords).xyz;
        if (gbufferidx == 1)
            gcolor = normalize(gcolor) * 0.5 + 0.5;
        if (gbufferidx == 2)
            gcolor = normalize(gcolor) * 0.5 + 0.5;
        FragColor = vec4(gcolor, 1.0);
    }
}
