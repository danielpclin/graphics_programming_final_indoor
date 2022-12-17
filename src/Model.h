#ifndef GRAPHICS_PROGRAMMING_MODEL_H
#define GRAPHICS_PROGRAMMING_MODEL_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <cmath>

#include "GL/glew.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class Model {
public:
	struct Mesh {
		GLuint vao;
		unsigned int indicesCount;
		unsigned int materialID;
	};
	struct Texture {
		GLuint textureID;
	};
	std::unordered_map<int, Texture> diffuse;
private:

	std::string directory;
	static Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
	void processNode(aiNode *node, const aiScene *scene);
	void processMaterial(const aiScene *scene);
	Texture loadTexture(std::string const &pFile);

public:
	std::vector<Mesh> meshes;
	explicit Model(std::string const &pFile);
};
#endif //GRAPHICS_PROGRAMMING_MODEL_H
