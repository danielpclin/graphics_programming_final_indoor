#ifndef TEXTURE_H
#define TEXTURE_H

#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <string>

class Texture
{
private:
    struct TextureData {
        TextureData();

        int width;
        int height;
        unsigned char* data;
    };

public:
    GLuint texture = 0;

    // load a png image and return a TextureData structure with raw data
    // not limited to png format. works with any image format that is RGBA-32bit
    static TextureData loadImg(const std::string& imgFilePath);

    explicit Texture(const std::string& filename);
    ~Texture();

    void bind(unsigned int slot) const;

    static void unbind();
};
#endif //TEXTURE_H
