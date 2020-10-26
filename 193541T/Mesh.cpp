#include "Mesh.h"

int Mesh::primitiveRestartIndex = 0;
std::vector<unsigned char> Mesh::heightMap;

static glm::vec3 RotateAbtYAxis(const glm::vec3& vec, const float& angleInRad){
    return glm::vec3(vec.x * cos(angleInRad) + vec.z * sin(angleInRad), vec.y, vec.x * -sin(angleInRad) + vec.z * cos(angleInRad));
}

Mesh::Mesh(const std::vector<Vertex>* const& vertices, const std::vector<uint>* const& indices) noexcept{
    VAO = VBO = EBO = 0;
    if(vertices){
        this->vertices = *vertices;
    }
    this->indices = const_cast<std::vector<uint>* const&>(indices);
    modelMatrices = nullptr;
}

Mesh::Mesh(const Mesh& other) noexcept{
    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;
    vertices = other.vertices;
    if(this != &other){
        indices = new std::vector<uint>(*(other.indices));
    }
    textures = other.textures;
    modelMatrices = nullptr;
}

Mesh::~Mesh() noexcept{
    delete indices;
    delete[] modelMatrices;
    glDeleteBuffers(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

Mesh* Mesh::CreatePts(){
    Mesh* mesh = new Mesh;
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, 0.f), glm::vec4(1.f, 0.f, 0.f, 1.f), glm::vec2(0.f), glm::vec3(0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, 1.f, 0.f), glm::vec4(0.f, 1.f, 0.f, 1.f), glm::vec2(0.f), glm::vec3(0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, -1.f, 0.f), glm::vec4(0.f, 0.f, 1.f, 1.f), glm::vec2(0.f), glm::vec3(0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, 0.f), glm::vec4(1.f, 1.f, 0.f, 1.f), glm::vec2(0.f), glm::vec3(0.f)});
    mesh->vertices.push_back({glm::vec3(0.f, 0.f, 0.f), glm::vec4(1.f, 0.f, 1.f, 1.f), glm::vec2(0.f), glm::vec3(0.f)});
    return mesh;
}

Mesh* Mesh::CreateQuad(){
    Mesh* mesh = new Mesh;
    glm::vec3 pos0(-1.f, -1.f, 0.f);
    glm::vec3 pos1(1.f, 1.f, 0.f);
    glm::vec3 pos2(-1.f, 1.f, 0.f);
    glm::vec3 pos3(1.f, -1.f, 0.f);
    glm::vec2 uv0(0.f, 0.f);
    glm::vec2 uv1(1.f, 1.f);
    glm::vec2 uv2(0.f, 1.f);
    glm::vec2 uv3(1.f, 0.f);

    glm::vec3 edge1 = pos1 - pos0;
    glm::vec3 edge2 = pos2 - pos0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;
    float f = 1.f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent, bitangent; //For each vertex //T and B lie on the same plane as normal map surface and align with tex axes U and V so calc them with vertices (to get edges of...) and texCoords (since in tangent space) of primitives
    tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    tangent = glm::normalize(tangent);

    //bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    //bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    //bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    //bitangent = glm::normalize(bitangent);

    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, 0.f), glm::vec4(0.f, 0.f, 1.f, 1.f), glm::vec2(0.f, 0.f), glm::vec3(0.f, 0.f, 1.f), tangent});
    mesh->vertices.push_back({glm::vec3(1.f, 1.f, 0.f), glm::vec4(0.f, 1.f, 1.f, 1.f), glm::vec2(1.f, 1.f), glm::vec3(0.f, 0.f, 1.f), tangent});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, 0.f), glm::vec4(1.f, 0.f, 0.f, 1.f), glm::vec2(0.f, 1.f), glm::vec3(0.f, 0.f, 1.f), tangent});
    mesh->vertices.push_back({glm::vec3(1.f, -1.f, 0.f), glm::vec4(0.f, 1.f, 0.f, 1.f), glm::vec2(1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), tangent});
    mesh->indices = new std::vector<uint>{0, 1, 2, 0, 3, 1};
    return mesh;
}

Mesh* Mesh::CreateCube(){
    Mesh* mesh = new Mesh;

    mesh->vertices.push_back({glm::vec3(1.f, 1.f, 1.f), glm::vec4(1.f), glm::vec2(1.f, 0.f), glm::vec3(0.f, 1.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, 1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 1.f), glm::vec3(0.f, 1.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, -1.f), glm::vec4(1.f), glm::vec2(0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)});

    mesh->vertices.push_back({glm::vec3(1.f, -1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 0.f), glm::vec3(1.f, 0.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, -1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 0.f), glm::vec3(1.f, 0.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, 1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 1.f), glm::vec3(1.f, 0.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, 1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 1.f), glm::vec3(1.f, 0.f, 0.f)});

    mesh->vertices.push_back({glm::vec3(1.f, 1.f, 1.f), glm::vec4(1.f), glm::vec2(1.f, 1.f), glm::vec3(0.f, 0.f, 1.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 1.f), glm::vec3(0.f, 0.f, 1.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 0.f), glm::vec3(0.f, 0.f, 1.f)});
    mesh->vertices.push_back({glm::vec3(1.f, -1.f, 1.f), glm::vec4(1.f), glm::vec2(1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)});

    mesh->vertices.push_back({glm::vec3(1.f, -1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, -1.f), glm::vec4(1.f), glm::vec2(0.f, 0.f), glm::vec3(0.f, 0.f, -1.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, -1.f), glm::vec4(1.f), glm::vec2(0.f, 1.f), glm::vec3(0.f, 0.f, -1.f)});
    mesh->vertices.push_back({glm::vec3(1.f, 1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 1.f), glm::vec3(0.f, 0.f, -1.f)});

    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 0.f), glm::vec3(-1.f, 0.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 0.f), glm::vec3(-1.f, 0.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, 1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 1.f), glm::vec3(-1.f, 0.f, 0.f)});

    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, -1.f), glm::vec4(1.f), glm::vec2(0.f, 1.f), glm::vec3(0.f, -1.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, -1.f, -1.f), glm::vec4(1.f), glm::vec2(1.f, 1.f), glm::vec3(0.f, -1.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(1.f, -1.f, 1.f), glm::vec4(1.f), glm::vec2(1.f, 0.f), glm::vec3(0.f, -1.f, 0.f)});
    mesh->vertices.push_back({glm::vec3(-1.f, -1.f, 1.f), glm::vec4(1.f), glm::vec2(0.f, 0.f), glm::vec3(0.f, -1.f, 0.f)});

    mesh->indices = new std::vector<uint>;
    short myArr[6]{0, 1, 2, 2, 3, 0};
    for(short i = 0; i < 6; ++i){
        for(short j = 0; j < 6; ++j){
            (*(mesh->indices)).push_back(i * 4 + myArr[j]);
        }
    }

    return mesh;
}

Mesh* Mesh::CreateSlicedTexQuad(const float& quadSize, const float& hTile, const float& vTile){
    Mesh* mesh = new Mesh;
    for(uint z = 0; z < (uint)quadSize; ++z){
        for(uint x = 0; x < (uint)quadSize; ++x){
            mesh->vertices.push_back({{float(x) / quadSize - .5f, 0.f, float(z) / quadSize - .5f}, glm::vec4(1.f), {float(x) / quadSize * hTile, 1.f - float(z) / quadSize * vTile}, {0.f, 1.f, 0.f}});
        }
    }
    mesh->indices = new std::vector<uint>;
    for(uint z = 0; z < uint(quadSize - 1.f); ++z){
        for(uint x = 0; x < uint(quadSize - 1.f); ++x){
            ///Triangle 1
            (*(mesh->indices)).push_back(uint(quadSize * z + x + 0));
            (*(mesh->indices)).push_back(uint(quadSize * (z + 1) + x + 0));
            (*(mesh->indices)).push_back(uint(quadSize * z + x + 1));

            ///Triangle 2
            (*(mesh->indices)).push_back(uint(quadSize * (z + 1) + x + 1));
            (*(mesh->indices)).push_back(uint(quadSize * z + x + 1));
            (*(mesh->indices)).push_back(uint(quadSize * (z + 1) + x + 0));
        }
    }
    return mesh;
}

void Mesh::LoadTex(const cstr& fPath, const str& type, const bool&& flipTex){
    Tex tex;
    const std::vector<cstr>* const fPaths = new std::vector<cstr>{fPath};
    tex.Create(GL_TEXTURE_2D, 999, 0, 0, GL_NEAREST, GL_LINEAR, GL_REPEAT, type, fPaths, flipTex);
    delete fPaths;
    textures.push_back(tex);
}

bool Mesh::LoadHeightMap(cstr file_path, std::vector<unsigned char>& heightMap){
    std::ifstream fileStream(file_path, std::ios::binary);
    if(!fileStream.is_open()){
        std::cout << "Impossible to open " << file_path << ". Are you in the right directory ?\n";
        return false;
    }

    fileStream.seekg(0, std::ios::end);
    std::streampos fsize = (long long)fileStream.tellg();

    fileStream.seekg(0, std::ios::beg);
    heightMap.resize((long long)fsize);
    fileStream.read((char*)&heightMap[0], fsize);

    fileStream.close();
    return true;
}

float Mesh::ReadHeightMap(std::vector<unsigned char> &heightMap, float x, float z){
    if(heightMap.size() == 0 || x <= -0.5f || x >= 0.5f || z <= -0.5f || z >= 0.5f){
        return 0.f;
    }

    const float SCALE_FACTOR = 256.f;
    long long terrainSize = (long long)sqrt((double)heightMap.size());
    long long zCoord = (long long)((z + 0.5) * terrainSize);
    long long xCoord = (long long)((x + 0.5) * terrainSize);

    return (float)heightMap[zCoord * terrainSize + xCoord] / SCALE_FACTOR;
}

Mesh* const Mesh::CreateHeightMap(cstr const& fPath, const float& hTile, const float& vTile){
    Mesh* mesh = new Mesh;
    if(!LoadHeightMap(fPath, heightMap)){
        exit(1);
    }
    const float SCALE_FACTOR = 256.f; //Set a scale factor to size terrain
    long long terrainSize = (long long)sqrt((double)heightMap.size()); //Calc the terrainSize

    std::vector<std::vector<glm::vec3>> pos = std::vector<std::vector<glm::vec3>>(terrainSize, std::vector<glm::vec3>(terrainSize));
    for(long long z = 0; z < terrainSize; ++z){
        for(long long x = 0; x < terrainSize; ++x){
            float scaledHeight = (float)heightMap[z * terrainSize + x] / SCALE_FACTOR; //Divide by SCALE_FACTOR to normalise
            pos[z][x] = glm::vec3(float(x) / terrainSize - .5f, scaledHeight, float(z) / terrainSize - .5f);
        }
    }

    std::vector<std::vector<glm::vec3>> normals = std::vector<std::vector<glm::vec3>>(terrainSize, std::vector<glm::vec3>(terrainSize));
    std::vector<std::vector<glm::vec3>> tempNormals[2];
    for(short i = 0; i < 2; ++i){
        tempNormals[i] = std::vector<std::vector<glm::vec3>>(terrainSize, std::vector<glm::vec3>(terrainSize));
    }
    for(long long z = 0; z < terrainSize - 1; ++z){
        for(long long x = 0; x < terrainSize - 1; ++x){
            const auto& vertexA = pos[z][x];
            const auto& vertexB = pos[z][x + 1];
            const auto& vertexC = pos[z + 1][x + 1];
            const auto& vertexD = pos[z + 1][x];
            const auto triangleNormalA = glm::cross(vertexB - vertexA, vertexA - vertexD);
            const auto triangleNormalB = glm::cross(vertexD - vertexC, vertexC - vertexB);
            tempNormals[0][z][x] = triangleNormalA.length() ? glm::normalize(triangleNormalA) : triangleNormalA;
            tempNormals[1][z][x] = triangleNormalB.length() ? glm::normalize(triangleNormalB) : triangleNormalB;
        }
    }
    for(long long z = 0; z < terrainSize; ++z){
        for(long long x = 0; x < terrainSize; ++x){
            const auto isFirstRow = z == 0;
            const auto isFirstColumn = x == 0;
            const auto isLastRow = z == terrainSize - 1;
            const auto isLastColumn = x == terrainSize - 1;
            auto finalVertexNormal = glm::vec3(0.f);
            if(!isFirstRow && !isFirstColumn){ //Look for triangle to the upper-left
                finalVertexNormal += tempNormals[0][z - 1][x - 1];
            }
            if(!isFirstRow && !isLastColumn){ //Look for triangles to the upper-right
                for(auto k = 0; k < 2; ++k){
                    finalVertexNormal += tempNormals[k][z - 1][x];
                }
            }
            if(!isLastRow && !isLastColumn){ //Look for triangle to the bottom-right
                finalVertexNormal += tempNormals[0][z][x];
            }
            if(!isLastRow && !isFirstColumn){ //Look for triangles to the bottom-right
                for(auto k = 0; k < 2; ++k){
                    finalVertexNormal += tempNormals[k][z][x - 1];
                }
            }
            normals[z][x] = finalVertexNormal.length() ? glm::normalize(finalVertexNormal) : finalVertexNormal; //Store final normal of j-th vertex in i-th row //Normalize to give avg of 4 normals
            mesh->vertices.push_back({pos[z][x], glm::vec4(1.f), glm::vec2(float(x) / terrainSize * hTile, 1.f - float(z) / terrainSize * vTile), normals[z][x]});
        }
    }

    mesh->indices = new std::vector<uint>;
    for(long long z = 0; z < terrainSize - 1; ++z){
        for(long long x = 0; x < terrainSize - 1; ++x){
            ///Triangle 1
            mesh->indices->emplace_back(uint(terrainSize * z + x + 0));
            mesh->indices->emplace_back(uint(terrainSize * (z + 1) + x + 0));
            mesh->indices->emplace_back(uint(terrainSize * z + x + 1));

            ///Triangle 2
            mesh->indices->emplace_back(uint(terrainSize * (z + 1) + x + 1));
            mesh->indices->emplace_back(uint(terrainSize * z + x + 1));
            mesh->indices->emplace_back(uint(terrainSize * (z + 1) + x + 0));
        }
    }
    return mesh;
}



void Mesh::Render(const int& primitive){
    if(primitive < 0){
        return (void)puts("Invalid primitive!\n");
    }
    for(uint i = 0; i < textures.size(); ++i){
        if(textures[i].GetActiveOnMesh()){
            if(textures[i].GetType() == "s"){
                ShaderProg::SetUni1i("useSpecular", 1, 0);
            }
            ShaderProg::UseTex(GL_TEXTURE_2D, textures[i], ("material." + textures[i].GetType() + "Map").c_str());
        }
    }

    if(!VAO){
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, colour));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, diffuseTexIndex));

        if(indices){
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(uint), &(*indices)[0], GL_STATIC_DRAW);
        }
        glBindVertexArray(0);
    }

    glBindVertexArray(VAO);
    indices ? glDrawElements(primitive, (int)indices->size(), GL_UNSIGNED_INT, nullptr) : glDrawArrays(primitive, 0, (int)vertices.size());
    glBindVertexArray(0);

    for(uint i = 0; i < textures.size(); ++i){
        if(textures[i].GetActiveOnMesh()){
            if(textures[i].GetType() == "s"){
                ShaderProg::SetUni1i("useSpecular", 0, 0);
            }
            ShaderProg::StopUsingTex(GL_TEXTURE_2D, textures[i]);
            textures[i].SetActiveOnMesh(0);
        }
    }
}

void Mesh::InstancedRender(const int& primitive, const std::vector<glm::mat4>& modelMats){
    if(primitive < 0){
        return (void)puts("Invalid primitive!\n");
    }
    for(uint i = 0; i < textures.size(); ++i){
        if(textures[i].GetActiveOnMesh()){
            if(textures[i].GetType() == "s"){
                ShaderProg::SetUni1i("useSpecular", 1, 0);
            }
            ShaderProg::UseTex(GL_TEXTURE_2D, textures[i], ("material." + textures[i].GetType() + "Map").c_str());
        }
    }

    if(!VAO){
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex) + modelMats.size() * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);
        glBufferSubData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), modelMats.size() * sizeof(glm::mat4), &(modelMats)[0]);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, colour));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, diffuseTexIndex));

        size_t mySize = vertices.size() * sizeof(Vertex);
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (const void*)(mySize));
        mySize += sizeof(glm::vec4);
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (const void*)(mySize));
        mySize += sizeof(glm::vec4);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (const void*)(mySize));
        mySize += sizeof(glm::vec4);
        glEnableVertexAttribArray(9);
        glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (const void*)(mySize));

        glVertexAttribDivisor(6, 1);
        glVertexAttribDivisor(7, 1);
        glVertexAttribDivisor(8, 1);
        glVertexAttribDivisor(9, 1);

        if(indices){
            glGenBuffers(1, &EBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(uint), &(*indices)[0], GL_STATIC_DRAW);
        }
        glBindVertexArray(0);
    }

    glBindVertexArray(VAO);
    indices ? glDrawElementsInstanced(primitive, (int)indices->size(), GL_UNSIGNED_INT, nullptr, (int)modelMats.size()) : glDrawArraysInstanced(primitive, 0, (int)vertices.size(), (int)modelMats.size());
    glBindVertexArray(0);

    for(uint i = 0; i < textures.size(); ++i){
        if(textures[i].GetActiveOnMesh()){
            if(textures[i].GetType() == "s"){
                ShaderProg::SetUni1i("useSpecular", 0, 0);
            }
            ShaderProg::StopUsingTex(GL_TEXTURE_2D, textures[i]);
            textures[i].SetActiveOnMesh(0);
        }
    }
}

void Mesh::BatchRender(const int& primitive, const std::vector<std::pair<std::tuple<glm::vec3, float, glm::vec3>, std::pair<glm::vec4, int>>>& vec){
    if(primitive < 0){
        return (void)puts("Invalid primitive!\n");
    }
    CreateQuads(vec);

    if(!VAO){
        glGenVertexArrays(1, &VAO);
    }
    glBindVertexArray(VAO);
    if(!VBO){
        glGenBuffers(1, &VBO); //A buffer manages a certain piece of GPU mem
        glBindBuffer(GL_ARRAY_BUFFER, VBO); //Makes batchVBO the buffer currently bound to the GL_ARRAY_BUFFER target, GL_ARRAY_BUFFER is batchVBO's type
        glBufferData(GL_ARRAY_BUFFER, 10000 * 4 * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW); //Max of 10000 quads //Can combine vertex attrib data into 1 arr or vec and fill batchVBO's mem with glBufferData

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, colour));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, diffuseTexIndex));
    }
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), &vertices[0]);

    if(!EBO && indices){
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 10000 * 6 * sizeof(uint), NULL, GL_DYNAMIC_DRAW); //Max of 10000 quads //Alloc/Reserve a piece of GPU mem and add data into it
    }
    if(EBO){
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices->size() * sizeof(uint), &(*indices)[0]);
        glDrawElements(primitive, (int)indices->size(), GL_UNSIGNED_INT, nullptr);
    } else{
        glDrawArrays(primitive, 0, (int)vertices.size());
    }
    glBindVertexArray(0);
}

void Mesh::CreateQuads(const std::vector<std::pair<std::tuple<glm::vec3, float, glm::vec3>, std::pair<glm::vec4, int>>>& vec){
    const size_t& mySize = vec.size();
    if(vertices.size()){
        vertices.clear();
    }
    if(!indices){
        indices = new std::vector<uint>();
    }
    indices->clear();
    const short myIndices[6]{0, 1, 2, 0, 3, 1};

    for(size_t i = 0; i < mySize; ++i){
        vertices.push_back({std::get<0>(vec[i].first) + RotateAbtYAxis(glm::vec3(-1.f, -1.f, 0.f) * std::get<2>(vec[i].first), std::get<1>(vec[i].first)), vec[i].second.first, glm::vec2(0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f), vec[i].second.second});
        vertices.push_back({std::get<0>(vec[i].first) + RotateAbtYAxis(glm::vec3(1.f, 1.f, 0.f) * std::get<2>(vec[i].first), std::get<1>(vec[i].first)), vec[i].second.first, glm::vec2(1.f, 1.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f), vec[i].second.second});
        vertices.push_back({std::get<0>(vec[i].first) + RotateAbtYAxis(glm::vec3(-1.f, 1.f, 0.f) * std::get<2>(vec[i].first), std::get<1>(vec[i].first)), vec[i].second.first, glm::vec2(0.f, 1.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f), vec[i].second.second});
        vertices.push_back({std::get<0>(vec[i].first) + RotateAbtYAxis(glm::vec3(1.f, -1.f, 0.f) * std::get<2>(vec[i].first), std::get<1>(vec[i].first)), vec[i].second.first, glm::vec2(1.f, 0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f), vec[i].second.second});
    }
    for(size_t j = 0; j < mySize; ++j){
        for(short k = 0; k < 6; ++k){
            (*indices).emplace_back(uint(myIndices[k] + 4 * j));
        }
    }
}