#include "Model.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Model::Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
    GLuint vao, vbo, ebo;
    // create buffers/arrays
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // load data into vertex buffers
    unsigned int vsize = 11;
    int vertices_size = sizeof(float) * vsize * mesh->mNumVertices;
    auto *vertices = (float*)malloc(vertices_size);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D vert = mesh->mVertices[i];
        aiVector3D norm = mesh->mNormals[i];
        aiVector3D tang = mesh->HasTangentsAndBitangents()? mesh->mTangents[i] : aiVector3D(0.0, 0.0, 0.0);
        aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0);

        //vertices
        vertices[i * vsize] = vert.x;
        vertices[i * vsize + 1] = vert.y;
        vertices[i * vsize + 2] = vert.z;

        //normals
        vertices[i * vsize + 3] = norm.x;
        vertices[i * vsize + 4] = norm.y;
        vertices[i * vsize + 5] = norm.z;

        //uvs
        vertices[i * vsize + 6] = uv.x;
        vertices[i * vsize + 7] = uv.y;

        //normals
        vertices[i * vsize + 8] = tang.x;
        vertices[i * vsize + 9] = tang.y;
        vertices[i * vsize + 10] = tang.z;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
    free(vertices);

    int indices_size = sizeof(unsigned int) * 3 * mesh->mNumFaces;
    auto *indices = (unsigned int*)malloc(indices_size);
    int face_count = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices < 3)
            continue;

        assert(face.mNumIndices == 3);

        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices[face_count * 3 + j] = face.mIndices[j];
        face_count++;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLuint)(face_count * 3 * sizeof(unsigned int)), indices, GL_STATIC_DRAW);
    free(indices);

    // set the vertex attribute pointers
    // vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * vsize, (GLvoid*)nullptr);
    // vertex normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * vsize, (GLvoid*)(sizeof(float) * 3));
    // vertex texture coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * vsize, (GLvoid*)(sizeof(float) * 6));
    // vertex tangents
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(float) * vsize, (GLvoid*)(sizeof(float) * 8));
    glBindVertexArray(0);
    std::cout << "Mesh loaded: " << mesh->mName.C_Str() << std::endl;
    return Mesh{ vao, 3 * mesh->mNumFaces, mesh->mMaterialIndex };
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

        meshes.push_back(processMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void Model::processMaterial(const aiScene *scene) {
    std::cout << "Material count: " << scene->mNumMaterials << std::endl;
    for (int i = 0; i < scene->mNumMaterials; i++)
    {
        Material material{};
        aiString name;
        scene->mMaterials[i]->Get(AI_MATKEY_NAME, name);
        std::cout << "Material " << i << " " << name.C_Str() << std::endl;
        std::cout << "Diffuse texture count " << scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) << std::endl;
        if (scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString str;
            scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            material.textureID = loadTexture(str.C_Str());
            material.hasTexture = true;
        }
        if (scene->mMaterials[i]->GetTextureCount(aiTextureType_HEIGHT) > 0) {
            aiString str;
            scene->mMaterials[i]->GetTexture(aiTextureType_HEIGHT, 0, &str);
            material.NormalMapID = loadNormalMap(str.C_Str());
            material.hasNormalMap = true;
        }

        aiColor3D color(0.f, 0.f, 0.f);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, color);
        std::cout << "Ambient color: " << glm::to_string(glm::vec3(color.r, color.g, color.b)) << std::endl;
        material.ambientColor = glm::vec3(color.r, color.g, color.b);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        std::cout << "Diffuse color: " << glm::to_string(glm::vec3(color.r, color.g, color.b)) << std::endl;
        material.diffuseColor = glm::vec3(color.r, color.g, color.b);
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, color);
        std::cout << "Specular color: " << glm::to_string(glm::vec3(color.r, color.g, color.b)) << std::endl;
        material.specularColor = glm::vec3(color.r, color.g, color.b);
        scene->mMaterials[i]->Get(AI_MATKEY_SHININESS, color);
        std::cout << "Shininess: " << color.r << std::endl;
        material.shininess = color.r;

        materials[i] = material;
    }
}

GLuint Model::loadTexture(const std::string &pFile) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load((directory + '/' + pFile).c_str(), &width, &height, &nrComponents, 4);
    if (data)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        std::cout << "Texture loaded: " << pFile << std::endl;
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << pFile << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

GLuint Model::loadNormalMap(std::string const& pFile)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load((directory + '/' + pFile).c_str(), &width, &height, &nrComponents, 4);
    if (data)
    {
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

       /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
       glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/

        std::cout << "Texture loaded: " << pFile << std::endl;
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << pFile << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

Model::Model(const std::string &pFile) {
    Assimp::Importer importer;
    const struct aiScene* scene = importer.ReadFile(pFile.c_str(),
                                               aiProcess_Triangulate |
                                               aiProcess_GenNormals |
                                               aiProcess_FlipUVs |
                                               aiProcess_CalcTangentSpace
//                                               aiProcess_GenUVCoords
//                                               aiProcess_SortByPType
    );

    // If the import failed, report it
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR ASSIMP " << importer.GetErrorString() << std::endl;
    }

    directory = pFile.substr(0, pFile.find_last_of('/'));

    // Now we can access the file's contents
    //DoTheSceneProcessing(scene);
    processNode(scene->mRootNode, scene);

    processMaterial(scene);
}
