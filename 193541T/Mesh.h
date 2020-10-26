#pragma once
#include "Buffer.h"
#include "Src.h"
#include "Utility.h"

class Mesh{ //Single drawable entity
public:
    Mesh(const std::vector<Vertex>* const& = nullptr, const std::vector<uint>* const& = nullptr) noexcept;
    Mesh(const Mesh&) noexcept; //Avoid shallow copy so won't delete a wild ptr
    virtual ~Mesh() noexcept;

    ///In local space (local to mesh)
    static Mesh* CreatePts();
    static Mesh* CreateQuad();
    static Mesh* CreateCube();
    static Mesh* CreateSlicedTexQuad(const float&, const float&, const float&);

    std::vector<Vertex> vertices;
    std::vector<uint>* indices;
    std::vector<Tex> textures;
    void LoadTex(const cstr&, const str&, const bool&& = 1);

    static int primitiveRestartIndex;
    static std::vector<unsigned char> heightMap;
    static bool LoadHeightMap(cstr, std::vector<unsigned char>&);
    static float ReadHeightMap(std::vector<unsigned char> &heightMap, float x, float z);
    static Mesh* const CreateHeightMap(cstr const&, const float& hTile, const float& vTile);


    virtual void Render(const int& primitive = GL_TRIANGLES);
    void InstancedRender(const int& primitive = GL_TRIANGLES, const std::vector<glm::mat4>& modelMats = {});
    void BatchRender(const int& primitive = GL_TRIANGLES, const std::vector<std::pair<std::tuple<glm::vec3, float, glm::vec3>, std::pair<glm::vec4, int>>>& vec = {});
protected:
    uint VAO, VBO, EBO;
private:
    glm::vec2 translations[100];
    glm::mat4* modelMatrices;


    void CreateQuads(const std::vector<std::pair<std::tuple<glm::vec3, float, glm::vec3>, std::pair<glm::vec4, int>>>& vec);
};