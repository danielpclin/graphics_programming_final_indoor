#include "TinyObjectModel.h"

TinyObjectModel::TinyObjectModel(const std::string &filename) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }
    if (!err.empty()) {
        std::cout << err << std::endl;
    }
    if (!ret) {
        exit(1);
    }

    std::vector<float> vertices, normals, tex_coords;  // if OBJ preserves vertex order, you can use element array buffer for memory efficiency
    for (auto & shape : shapes) {  // for 'ladybug.obj', there is only one object
        int index_offset = 0;
        for (int f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            int fv = shape.mesh.num_face_vertices[f];
            for (int v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 0]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 1]);
                vertices.push_back(attrib.vertices[3 * idx.vertex_index + 2]);
                normals.push_back(attrib.normals[3 * idx.normal_index + 0]);
                normals.push_back(attrib.normals[3 * idx.normal_index + 1]);
                normals.push_back(attrib.normals[3 * idx.normal_index + 2]);
                tex_coords.push_back(attrib.texcoords[2 * idx.texcoord_index + 0]);
                tex_coords.push_back(attrib.texcoords[2 * idx.texcoord_index + 1]);
            }
            index_offset += fv;
            vertexCount += fv;
        }
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + normals.size() * sizeof(float) + tex_coords.size() * sizeof(float), (GLvoid *) nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), normals.size() * sizeof(float), normals.data());
    glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float) + normals.size() * sizeof(float), tex_coords.size() * sizeof(float), tex_coords.data());

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(vertices.size() * sizeof(float) + normals.size() * sizeof(float)));
    glEnableVertexAttribArray(2);

    std::cout << "Loaded model \"" << filename << "\", " << vertexCount << " vertices" << std::endl;
}

void TinyObjectModel::bind() const {
    glBindVertexArray(vao);
}
