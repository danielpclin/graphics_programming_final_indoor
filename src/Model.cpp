#include "Model.h"

Model::Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
    GLuint vao, vbo, ebo;
    // create buffers/arrays
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // load data into vertex buffers
    int vertices_size = sizeof(float) * 8 * mesh->mNumVertices;
    float *vertices = (float*)malloc(vertices_size);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        aiVector3D vert = mesh->mVertices[i];
        aiVector3D norm = mesh->mNormals[i];
        aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0);

        //verts
        vertices[i * 8] = vert.x;
        vertices[i * 8 + 1] = vert.y;
        vertices[i * 8 + 2] = vert.z;

        //normals
        vertices[i * 8 + 3] = norm.x;
        vertices[i * 8 + 4] = norm.y;
        vertices[i * 8 + 5] = norm.z;

        //uvs
        vertices[i * 8 + 6] = uv.x;
        vertices[i * 8 + 7] = uv.y;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
    free(vertices);

    int indices_size = sizeof(int) * 3 * mesh->mNumFaces;
    auto *indices = (unsigned int*)malloc(indices_size);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices[i * 3 + j] = face.mIndices[j];
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);
    free(indices);

    // set the vertex attribute pointers
    // vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
    // vertex texture coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 6));
    // vertex normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));

    glBindVertexArray(0);
    std::cout << "mesh loaded: " << mesh->mName.C_Str() << std::endl;
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

        std::cout << "Material " << i << " Diffuse " << scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) << " height " << scene->mMaterials[i]->GetTextureCount(aiTextureType_HEIGHT) << std::endl;
        if (scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString str;
            scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &str);
            diffuse[i] = loadTexture(str.C_Str());
        }
    }
}

Model::Texture Model::loadTexture(const std::string &pFile) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load((directory + '/' + pFile).c_str(), &width, &height, &nrComponents, 4);
    if (data)
    {
        glActiveTexture(GL_TEXTURE0 + textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
    return Texture{ textureID };
}

Model::Model(const std::string &pFile) {
    const struct aiScene* scene = aiImportFile(pFile.c_str(),
                                               aiProcess_Triangulate |
                                               aiProcess_JoinIdenticalVertices |
                                               aiProcess_GenNormals |
                                               aiProcess_FlipUVs |
                                               aiProcess_GenUVCoords|
                                               aiProcess_SortByPType
    );

    // If the import failed, report it
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        //DoTheErrorLogging(aiGetErrorString());
        std::cout << "ERROR ASSIMP " << aiGetErrorString() << std::endl;
    }

    directory = pFile.substr(0, pFile.find_last_of('/'));

    // Now we can access the file's contents
    //DoTheSceneProcessing(scene);
    processNode(scene->mRootNode, scene);

    processMaterial(scene);

    // We're done. Release all resources associated with this import
    aiReleaseImport(scene);
}
