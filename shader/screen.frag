#version 410
layout(location = 0) out vec4 FragColor;

in vec2 TexCoords;

struct Config {
    bool blinnPhong;
    bool directionalLightShadow;
    bool bloom;
    bool deferredShading;
    bool normalMapping;
};
uniform Config config;

uniform sampler2D colorTexture;
uniform sampler2D BloomEffect_HDR_Texture;
uniform sampler2D BloomEffect_Blur_Texture;

/*----- Deferred Shading -----*/
//g buffers
uniform sampler2D gtex[5];

uniform int gbufferidx;



void main()
{
	vec3 color = vec3(texture(colorTexture, TexCoords));

    FragColor = vec4(color, 1.0);

    /*----- Bloom Effect Begin ----- */
    if (config.bloom) {
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
        FragColor = vec4(color + result, 1.0);
    }

    if (config.deferredShading) {
        vec3 gcolor = texture(gtex[gbufferidx], TexCoords).xyz;
        if (gbufferidx == 1)
            gcolor = normalize(gcolor) * 0.5 + 0.5;
        if (gbufferidx == 2)
            gcolor = normalize(gcolor) * 0.5 + 0.5;
        FragColor = vec4(gcolor, 1.0);
    }
    /*----- Bloom Effect End -----*/


}
