#ifndef GRAPHICS_PROGRAMMING_TINY_OBJECT_MODEL_H
#define GRAPHICS_PROGRAMMING_TINY_OBJECT_MODEL_H
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cmath>

#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

class TinyObjectModel
{
public:
    GLuint vao = 0;			// vertex array object
    GLuint vbo = 0;			// vertex buffer object
    int vertexCount = 0;

    // Load .obj model
    explicit TinyObjectModel(const std::string& filename);

    void bind() const;
};
#endif //GRAPHICS_PROGRAMMING_TINY_OBJECT_MODEL_H
