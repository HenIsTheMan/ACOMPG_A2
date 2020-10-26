#pragma once
#include "Buffer.h"
#include "Cam.h"
#include "Mesh.h"
#include "Model.h"
#include "ShaderProg.h"
#include "SpriteAni.h"
#include "Entity.h"

class Scene final{
    glm::vec3 quadPos[5]{
        glm::vec3(1.5f, 1.0f, -10.5f),
        glm::vec3(1.5f, 1.5f, 2.5f),
        glm::vec3(0.0f, 12.f, -5.f),
        glm::vec3(-0.5f, 1.0f, -6.f),
        glm::vec3(0.5f, -1.5f, -1.5f)
    };
    std::vector<cstr> texFaces{
        "Resources/Textures/Skyboxes/Right.png",
        "Resources/Textures/Skyboxes/Left.png",
        "Resources/Textures/Skyboxes/Top.png",
        "Resources/Textures/Skyboxes/Bottom.png",
        "Resources/Textures/Skyboxes/Front.png",
        "Resources/Textures/Skyboxes/Back.png"
    };
    bool canPutOutFire;
    bool showFire;
    bool showTerrainNormals;
    float elapsedTime;
    float fogBT;
    float terrainNormalsBT;
    short fogType;
	Mesh* meshes[6];
    Model* models[15];
	ShaderProg* basicShaderProg;
    ShaderProg* blurShaderProg;
	ShaderProg* explosionShaderProg;
	ShaderProg* outlineShaderProg;
	ShaderProg* normalsShaderProg;
	ShaderProg* quadShaderProg;
	ShaderProg* screenQuadShaderProg;
    SpriteAnimation* spriteAni;
    Tex cubemap;
    UniBuffer* magnitudeStorer;
    UniBuffer* brightnessStorer;
    void RenderCampfire(const Cam& cam);
    void RenderTerrainNormals(const Cam&) const;
    void RenderSky(const Cam&, const bool&&) const;
    void RenderTreesAndRocks(const Cam& cam) const;
    void SetUnis(const Cam&, const short& = 0, const glm::vec3& = glm::vec3(0.f), const glm::vec4& = {0.f, 1.f, 0.f, 0.f}, const glm::vec3& = glm::vec3(1.f)) const;



    EntityChief particleSystem;
    float rainBT;
    float smokeBT;
    float swirlBT;
    std::vector<glm::mat4> grassModelMats;
public:
	Scene();
	~Scene();
    void Init();
    void Update(Cam const&);



    void MyRender(Cam const& cam, const Tex* const& enCubemap, const Tex* const& depthTexs);
    void BlurRender(const uint& brightTexRefID, const bool& horizontal);
    void DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID, const int& typePPE, const bool& lineariseDepth, const glm::vec3& translate, const glm::vec3& scale) const;
};