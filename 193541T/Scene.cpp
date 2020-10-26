#include "Scene.h"
#include "LightChief.h"
#define NEAR_D .1f
#define FAR_D 500.f
#define NEAR_S .1f
#define FAR_S 400.f

extern float angularFOV;
extern float dt;

Scene::Scene():
    meshes{
        Mesh::CreatePts(),
        Mesh::CreateQuad(),
        Mesh::CreateCube(),
        Mesh::CreateHeightMap("Resources/Textures/HeightMaps/hMap.raw", 8.f, 8.f),
        Mesh::CreateSlicedTexQuad(24.f, 2.f, 2.f),
        new Mesh()
    },
    models{ new Model("Resources/Models/Skydome.obj"),
            new Model("Resources/Models/nanosuit.obj"),
            new Model("Resources/Models/Rock0.obj"),
            new Model("Resources/Models/Rock1.obj"),
            new Model("Resources/Models/Rock2.obj"),
            new Model("Resources/Models/Tree0.obj"),
            new Model("Resources/Models/Tree1.obj"),
            new Model("Resources/Models/Tree2.obj"),
            new Model("Resources/Models/Tree3.obj"),
            new Model("Resources/Models/Tree4.obj"),
            new Model("Resources/Models/Campfire.obj"),
            new Model("Resources/Models/Tent.obj"),
            new Model("Resources/Models/House.obj"),
            new Model("Resources/Models/Sword.obj"),
            new Model("Resources/Models/Wolf.obj") },
    basicShaderProg(new ShaderProg("Resources/Shaders/Basic.vs", "Resources/Shaders/Basic.fs")),
    blurShaderProg(new ShaderProg("Resources/Shaders/Blur.vs", "Resources/Shaders/Blur.fs")),
    explosionShaderProg(new ShaderProg("Resources/Shaders/Basic.vs", "Resources/Shaders/Basic.fs", "Resources/Shaders/Explosion.gs")),
    outlineShaderProg(new ShaderProg("Resources/Shaders/Basic.vs", "Resources/Shaders/Outline.fs")),
    normalsShaderProg(new ShaderProg("Resources/Shaders/Basic.vs", "Resources/Shaders/Outline.fs", "Resources/Shaders/Normals.gs")),
    quadShaderProg(new ShaderProg("Resources/Shaders/Basic.vs", "Resources/Shaders/Quad.fs")),
    screenQuadShaderProg(new ShaderProg("Resources/Shaders/Basic.vs", "Resources/Shaders/ScreenQuad.fs"))
{
    Init();
}

Scene::~Scene(){
    if(spriteAni){
        delete spriteAni;
        spriteAni = nullptr;
    }
    for(short i = 0; i < sizeof(meshes) / sizeof(meshes[0]); ++i){
        delete meshes[i];
    }
    for(short i = 0; i < sizeof(models) / sizeof(models[0]); ++i){
        delete models[i];
    }
    cubemap.Del();
    delete magnitudeStorer;
    delete brightnessStorer;
    delete basicShaderProg;
    delete blurShaderProg;
    delete explosionShaderProg;
    delete outlineShaderProg;
    delete normalsShaderProg;
    delete quadShaderProg;
    delete screenQuadShaderProg;
}

void Scene::RenderCampfire(const Cam& cam){
    SetUnis(cam, 2, glm::vec3(-150.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -150.f / 500.f, -10.f / 500.f), -10.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(2.f));
    models[10]->Render(GL_TRIANGLES, true);

    if(showFire){
        //glStencilFunc(GL_ALWAYS, 1, 0xFF) //Default
        glStencilMask(0xFF); //Set bitmask that is ANDed with stencil value abt to be written to stencil buffer //Each bit is written to the stencil buffer unchanged (bitmask of all 1s [default])
        const bool&& mergeBorders = false;
        const glm::vec3&& translate = glm::vec3(-150.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -150.f / 500.f, -10.f / 500.f) + 17.5f, -10.f);
        const glm::vec3&& scale = glm::vec3(20.f, 40.f, 20.f);
        canPutOutFire = false;

        quadShaderProg->Use();
        SetUnis(cam, 0, translate, {0.f, 1.f, 0.f, glm::degrees(atan2(cam.GetPos().x + 150.f, cam.GetPos().z + 10.f))}, scale);
        ShaderProg::UseTex(GL_TEXTURE_2D, spriteAni->textures[0], "texSampler");
        spriteAni->Render(GL_TRIANGLES);
        ShaderProg::StopUsingTex(GL_TEXTURE_2D, spriteAni->textures[0]);

        glm::vec3 normal = glm::vec3(glm::rotate(glm::mat4(1.f), glm::degrees(atan2(cam.GetPos().x + 150.f, cam.GetPos().z + 10.f)), glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(0.f, 0.f, 1.f, 1.f));
        float dist = glm::dot(translate, normal);
        glm::vec3 pt = cam.GetPos();
        glm::vec3 dir = glm::normalize(cam.CalcFront());

        float denom = glm::dot(dir, normal);
        if(abs(denom) > 0.0001f){
            float lambda = (dist - glm::dot(pt, normal)) / denom;
            glm::vec3 intersectionPt = pt + lambda * dir;

            ///Prevent intersection with transparent parts
            glm::vec3 planeMaskTranslate = translate - glm::vec3(0.f, 3.f, 0.f);
            glm::vec3 planeMaskScale = glm::vec3(8.f, 12.5f, 1.f);

            if(lambda != 0.f && intersectionPt.x >= planeMaskTranslate.x + -planeMaskScale.x && intersectionPt.x <= planeMaskTranslate.x + planeMaskScale.x &&
                intersectionPt.y >= planeMaskTranslate.y + -planeMaskScale.y && intersectionPt.y <= planeMaskTranslate.y + planeMaskScale.y){
                canPutOutFire = true;
                if(mergeBorders){
                    glDepthMask(GL_FALSE);
                }
                glStencilFunc(GL_NOTEQUAL, 1, 0xFF); //The frag passes... and is drawn if its ref value of 1 is not equal to stencil value in the stencil buffer //++params??
                outlineShaderProg->Use();
                ShaderProg::SetUni3f("myRGB", glm::vec3(1.f));
                ShaderProg::SetUni1f("myAlpha", .5f);
                SetUnis(cam, 0, translate, {0.f, 1.f, 0.f, glm::degrees(atan2(cam.GetPos().x + 150.f, cam.GetPos().z + 10.f))}, scale + glm::vec3(5.f));
                ShaderProg::UseTex(GL_TEXTURE_2D, spriteAni->textures[0], "outlineTex");
                spriteAni->Render(GL_TRIANGLES);
                ShaderProg::StopUsingTex(GL_TEXTURE_2D, spriteAni->textures[0]);
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                if(mergeBorders){
                    glDepthMask(GL_TRUE);
                } else{
                    glClear(GL_STENCIL_BUFFER_BIT);
                }
            }
        }
    }
}

void Scene::RenderTerrainNormals(const Cam& cam) const{ //Can use to add fur //Wrong normals due to incorrectly loading vertex data, improperly specifying vertex attributes or incorrectly managing them in the shaders
    normalsShaderProg->Use();
    ShaderProg::SetUni1f("len", 10.f);
    ShaderProg::SetUni3f("myRGB", .3f, .3f, .3f);
    ShaderProg::SetUni1f("myAlpha", 1.f);
    ShaderProg::SetUni1i("drawNormals", 1);
    SetUnis(cam, 0, glm::vec3(0.f, -100.f, 0.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(500.f, 100.f, 500.f));
    meshes[3]->Render(GL_TRIANGLES);
    ShaderProg::SetUni1i("drawNormals", 0);
}

void Scene::RenderSky(const Cam& cam, const bool&& type) const{
    if(type){
        basicShaderProg->Use();
        ShaderProg::SetUni1i("skydome", 1);
        ShaderProg::LinkUniBlock("Settings", 1);
        SetUnis(cam, 1, glm::vec3(0.f, -200.f, 0.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(150.f, 100.f, 150.f));
        models[0]->Render(GL_TRIANGLES, true);
        SetUnis(cam, 1, glm::vec3(0.f, 200.f, 0.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(150.f, -100.f, 150.f));
        glCullFace(GL_FRONT);
        models[0]->Render(GL_TRIANGLES, true);
        glCullFace(GL_BACK);
        ShaderProg::SetUni1i("skydome", 0);
    } else{
        glDepthFunc(GL_LEQUAL); //Modify comparison operators used for depth test //Depth buffer filled with 1.0s for the skybox so must ensure the skybox passes the depth tests with depth values <= that in the depth buffer??
        glFrontFace(GL_CW);
        //glDepthMask(GL_FALSE); //Need glDepthMask (perform depth test on all fragments but not update the depth buffer [use read-only depth buffer] if GL_FALSE) if skybox drawn as 1st obj as it's only 1x1x1

        basicShaderProg->Use();
        SetUnis(cam, 1);
        ShaderProg::SetUni1i("cubemap", 1);
        ShaderProg::UseTex(GL_TEXTURE_CUBE_MAP, cubemap, "cubemapSampler");
        meshes[2]->Render(GL_TRIANGLES);
        ShaderProg::StopUsingTex(GL_TEXTURE_CUBE_MAP, cubemap);
        ShaderProg::SetUni1i("cubemap", 0);

        //glDepthMask(GL_TRUE); //Skybox is likely to fail most depth tests and hence fail to render as it's 1x1x1 but cannot just disable depth test as skybox will overwrite all opaque objs
        glFrontFace(GL_CCW);
        glDepthFunc(GL_LESS);
    }
}

void Scene::RenderTreesAndRocks(const Cam& cam) const{
    SetUnis(cam, 2, glm::vec3(107.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 107.f / 500.f, 50.f / 500.f) - 5.f, 50.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[2]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-35.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -35.f / 500.f, 80.f / 500.f) - 5.f, 80.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(10.f));
    models[3]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(20.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 20.f / 500.f, -100.f / 500.f) - 5.f, -100.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(10.f));
    models[3]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(100.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 100.f / 500.f, -50.f / 500.f) - 5.f, -50.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[4]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-10.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -10.f / 500.f, 170.f / 500.f) - 5.f, 170.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[5]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(30.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 30.f / 500.f, 210.f / 500.f) - 5.f, 210.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[5]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-100.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -100.f / 500.f, 210.f / 500.f) - 5.f, 210.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[5]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-80.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -80.f / 500.f, -210.f / 500.f) - 5.f, -210.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[5]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-20.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -20.f / 500.f, -220.f / 500.f) - 5.f, -220.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[6]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(70.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 70.f / 500.f, -170.f / 500.f) - 5.f, -170.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[6]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-230.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -230.f / 500.f, -70.f / 500.f) - 5.f, -70.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[6]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-210.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -210.f / 500.f, 40.f / 500.f) - 5.f, 40.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[6]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(30.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 80.f / 500.f, -150.f / 500.f) - 5.f, -150.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[7]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(200.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 200.f / 500.f, -50.f / 500.f) - 5.f, -50.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[7]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(170.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 170.f / 500.f, 110.f / 500.f) - 5.f, 110.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[7]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(180.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 180.f / 500.f, 70.f / 500.f) - 5.f, -100.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[8]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-230.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -230.f / 500.f, 70.f / 500.f) - 5.f, 70.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[8]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(220.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 220.f / 500.f, 200.f / 500.f) - 5.f, 200.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[8]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(150.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 150.f / 500.f, 220.f / 500.f) - 5.f, 220.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[8]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-220.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -220.f / 500.f, -200.f / 500.f) - 5.f, -200.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[9]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-150.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -150.f / 500.f, -220.f / 500.f) - 5.f, -220.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[9]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-120.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -120.f / 500.f, 160.f / 500.f) - 5.f, 160.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[9]->Render(GL_TRIANGLES, true);

    SetUnis(cam, 2, glm::vec3(-150.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 150.f / 500.f, 130.f / 500.f) - 5.f, -130.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(30.f));
    models[9]->Render(GL_TRIANGLES, true);
}

void Scene::Init(){
    cubemap.Create(GL_TEXTURE_CUBE_MAP, 999, 0, 0, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, "skybox", &texFaces, 0);
    meshes[1]->LoadTex("Resources/Textures/blending_transparent_window.png", "d");
    meshes[1]->LoadTex("Resources/Textures/grass.png", "d");
    meshes[1]->LoadTex("Resources/Textures/smoke.tga", "d");
    meshes[2]->LoadTex("Resources/Textures/container.png", "d");
    meshes[2]->LoadTex("Resources/Textures/containerSpecC.png", "s");
    meshes[2]->LoadTex("Resources/Textures/matrix.jpg", "e");
    meshes[3]->LoadTex("Resources/Textures/GrassGround.jpg", "d");
    meshes[3]->LoadTex("Resources/Textures/Rocky.jpg", "d");
    meshes[3]->LoadTex("Resources/Textures/Snowy.jpg", "d");
    meshes[4]->LoadTex("Resources/Textures/Water.jpg", "d");

    spriteAni = SpriteAnimation::CreateSpriteAni(4, 8);
    spriteAni->LoadTex("Resources/Textures/fire.png", "d");
    spriteAni->AddAnimation("Animation3", 0, 32);
    spriteAni->PlayAnimation("Animation3", -1, .5f);

    glPointSize(50.f);
    glLineWidth(2.f);
    LightChief::CreateLightD(glm::vec3(0.f, -1.f, 0.f));
    LightChief::CreateLightS(glm::vec3(0.f), glm::vec3(0.f), glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(17.5f)));

    canPutOutFire = false;
    showFire = true;
    showTerrainNormals = false;
    elapsedTime = fogBT = terrainNormalsBT = 0.f;
    fogType = 0;
    magnitudeStorer = new UniBuffer(1.3f * 0.f, 0);
    brightnessStorer = new UniBuffer(.7f, 1);



    rainBT = 4.f;
    smokeBT = 4.f;
    swirlBT = 4.f;
    for(int i = 0; i < 500; ++i){
        particleSystem.AddEntity();
    }

    glm::mat4 model;
    for(uint i = 0; i < 999; ++i){
        const float xPos = float(rand() % (501 - 4)) - 248.f;
        const float zPos = float(rand() % (501 - 4)) - 248.f;
        model = glm::translate(glm::mat4(1.f), glm::vec3(xPos, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, xPos / 500.f, zPos / 500.f) + 4.f, zPos));
        model = glm::scale(model, glm::vec3(4.f));
        grassModelMats.emplace_back(model);
    }
}

void Scene::Update(Cam const& cam){
    elapsedTime += dt;
    if(GetAsyncKeyState(VK_RBUTTON) & 0x8001 && canPutOutFire){
        showFire = false;
    }
    if(GetAsyncKeyState(VK_SPACE) & 0x8000 && terrainNormalsBT <= elapsedTime){
        showTerrainNormals = !showTerrainNormals;
        terrainNormalsBT = elapsedTime + .5f;
    }
    if(GetAsyncKeyState(VK_RETURN) & 0x8000 && fogBT <= elapsedTime){
        ++fogType;
        if(fogType == 3){
            fogType = -1;
        }
        fogBT = elapsedTime + .5f;
    }
    if(LightChief::sLights.size()){
        LightChief::sLights[0].pos = cam.GetPos();
        LightChief::sLights[0].dir = cam.CalcFront();
    }
    spriteAni->Update();



    if(rainBT <= elapsedTime){
        for(short i = 0; i < 10; ++i){
            Entity* const& entity = particleSystem.FetchEntity();
            entity->SetAttribs({
                Entity::EntityType::Rain,
                true,
                0.f,
                glm::vec4(0.f, .4f, 1.f, 1.f),
                glm::vec3(.5f, 5.f, 1.f),
                glm::vec3(0.f),

                glm::vec3(float(rand() % (501 - 4)) - 248.f, 500.f, float(rand() % (501 - 4)) - 248.f),
                glm::vec3(0.f),
                .01f,
                glm::vec3(0.f, -2.f, 0.f),

                glm::vec3(0.f, -1.f, 0.f),
                0.f,
                0.f,
                glm::vec3(0.f)
            });
            if(!entity->GetMesh()){
                entity->SetMesh(Mesh::CreateQuad());
            }
        }
        rainBT = elapsedTime + dt;
    }
    if(smokeBT <= elapsedTime){
        Entity* const& entity = particleSystem.FetchEntity();
        entity->SetAttribs({
            Entity::EntityType::Smoke,
            true,
            3.f,
            glm::vec4(glm::vec3(0.f), 1.f),
            glm::vec3(7.f, 7.f, 1.f),
            glm::vec3(0.f),

            glm::vec3(-150.f + (float(rand() % 3) - 1.f) * 3.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -150.f / 500.f, -10.f / 500.f) + 30.f, -10.f + (float(rand() % 3) - 1.f) * 3.f),
            glm::vec3(0.f),
            .005f,
            glm::vec3(0.f, .05f, 0.f),

            glm::vec3(0.f, 1.f, 0.f),
            0.f,
            0.f,
            glm::vec3(0.f)
        });
        if(!entity->GetMesh()){
            entity->SetMesh(Mesh::CreateQuad());
        }
        smokeBT = elapsedTime + dt;
    }
    if(swirlBT <= elapsedTime){
        Entity* const& entity = particleSystem.FetchEntity();
        entity->SetAttribs({
            Entity::EntityType::Swirl,
            true,
            360.f * 3.5f,
            glm::vec4(glm::vec3(10.f, 15.f, 10.f), 1.f),
            glm::vec3(1.f),
            glm::vec3(0.f),

            glm::vec3(0.f, -25.f, 0.f),
            glm::vec3(0.f),
            .005f,
            glm::vec3(0.f),

            glm::vec3(0.f),
            0.f,
            0.f,
            glm::vec3(0.f)
            });
        if(!entity->GetMesh()){
            entity->SetMesh(Mesh::CreateQuad());
        }
        swirlBT = elapsedTime + dt * 5.f;
    }

    const size_t& mySize = particleSystem.RetrieveEntityPool().size();
    for(size_t i = 0; i < mySize; ++i){
        Entity* const& entity = particleSystem.RetrieveEntityPool()[i];
        if(entity->attribs.active){
            switch(entity->attribs.type){
                case Entity::EntityType::Rain:
                    entity->attribs.vel += entity->attribs.force / entity->attribs.mass * dt;
                    entity->attribs.pos += entity->attribs.vel * dt;

                    if(entity->attribs.pos.y <= -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, entity->attribs.pos.x / 500.f, entity->attribs.pos.z / 500.f) - entity->attribs.scale.y / 2.f){
                        entity->attribs.active = false;
                    }
                    break;
                case Entity::EntityType::Smoke:
                    entity->attribs.vel += entity->attribs.force / entity->attribs.mass * dt;
                    entity->attribs.pos += entity->attribs.vel * dt;
                    entity->attribs.life -= dt;
                    if(entity->attribs.life <= 0.f){
                        entity->attribs.active = false;
                    }
                    break;
                case Entity::EntityType::Swirl:
                    entity->attribs.life -= dt * 70.f;
                    if(entity->attribs.life <= 0.f){
                        entity->attribs.active = false;
                    }
                    entity->attribs.pos = glm::vec3(
                        15.f * cos(glm::radians(360.f - entity->attribs.life)) - 20.f,
                        entity->attribs.pos.y + 5.f * dt,
                        15.f * sin(glm::radians(360.f - entity->attribs.life)) - 15.f
                    );
                    break;
            }
        }
    }
}

void Scene::SetUnis(const Cam& cam, const short& type, const glm::vec3& translate, const glm::vec4& rotate, const glm::vec3& scale) const{
    glm::mat4 model = glm::translate(glm::mat4(1.f), translate);
    model = glm::rotate(model, glm::radians(rotate.w), glm::vec3(rotate));
    model = glm::scale(model, scale);
    glm::mat4 view = (type & 1 ? glm::mat4(glm::mat3(cam.LookAt())) : cam.LookAt()); //Remove translation by taking upper-left 3x3 matrix of the 4x4 transformation matrix
    glm::mat4 projection; //Determines range of visibility through affecting which frags get clipped
    if(cam.GetProjectionIndex() == 0){
        projection = glm::ortho(-400.f, 400.f, -300.f, 300.f, NEAR_D, FAR_D); //No perspective deform of vertices of objs in scene as directional light rays are parallel
    } else if(cam.GetProjectionIndex() == 1){ //combine with 2??
        projection = glm::ortho(-400.f, 400.f, -300.f, 300.f, NEAR_S, FAR_S);
        //projection = glm::perspective(glm::radians(angularFOV), 1.f, NEAR, FAR);
    } else{
        //projection = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, 0.1f, 100.0f); //Ortho projection matrix produces clip coords that are NDC while perspective projection matrix produces clip coords with a range of -w to w
        projection = glm::perspective(glm::radians(angularFOV), cam.GetAspectRatio(), .1f, 9999.f); //++ long dist from near plane to...??
    }
    glm::mat4 MVP = projection * view * model;
    ShaderProg::SetUniMtx4fv("model", &model[0][0], 0); //Local coords in local/obj space => world coords in world space //SRT
    ShaderProg::SetUniMtx4fv("view", &view[0][0], 0); //World coords in world space => view coords in view/cam/eye space
    ShaderProg::SetUniMtx4fv("projection", &projection[0][0], 0); //View coords in view/cam/eye space => clip coords in clip space //Clipped vertices (not in clipping range/vol) are discarded when clipping occurs before frag shaders run
    ShaderProg::SetUniMtx4fv("MVP", glm::value_ptr(MVP), 0);
    ShaderProg::SetUni3f("camPos", cam.GetPos(), 0);

    if(type == 2){
        const size_t &&amtP = LightChief::pLights.size(), &&amtD = LightChief::dLights.size(), &&amtS = LightChief::sLights.size();
        ShaderProg::SetUni1i("amtP", (int)amtP);
        ShaderProg::SetUni1i("amtD", (int)amtD);
        ShaderProg::SetUni1i("amtS", (int)amtS);
        if(amtP){
            for(short i = 0; i < amtP; ++i){
                ShaderProg::SetUni3f(("pLights[" + std::to_string(i) + "].pos").c_str(), LightChief::pLights[i].pos);
                ShaderProg::SetUni1f(("pLights[" + std::to_string(i) + "].constant").c_str(), LightChief::pLights[i].constant);
                ShaderProg::SetUni1f(("pLights[" + std::to_string(i) + "].linear").c_str(), LightChief::pLights[i].linear);
                ShaderProg::SetUni1f(("pLights[" + std::to_string(i) + "].quadratic").c_str(), LightChief::pLights[i].quadratic);
            }
        }
        if(amtD){
            for(short i = 0; i < amtD; ++i){
                ShaderProg::SetUni3f(("dLights[" + std::to_string(i) + "].dir").c_str(), LightChief::dLights[i].dir);
            }
        }
        if(amtS){
            for(short i = 0; i < amtS; ++i){
                ShaderProg::SetUni3f(("sLights[" + std::to_string(i) + "].pos").c_str(), LightChief::sLights[i].pos);
                ShaderProg::SetUni3f(("sLights[" + std::to_string(i) + "].dir").c_str(), LightChief::sLights[i].dir);
                ShaderProg::SetUni1f(("sLights[" + std::to_string(i) + "].cosInnerCutoff").c_str(), LightChief::sLights[i].cosInnerCutoff);
                ShaderProg::SetUni1f(("sLights[" + std::to_string(i) + "].cosOuterCutoff").c_str(), LightChief::sLights[i].cosOuterCutoff);
            }
        }
        ShaderProg::SetUni1f("material.shininess", 32.f); //More light scattering if lower //Test low?? //Make abstract??
    }
}



void Scene::MyRender(Cam const& cam, const Tex* const& enCubemap, const Tex* const& depthTexs){
    glStencilMask(0x00); //Can make outlines overlap

    if(!depthTexs){
        basicShaderProg->Use();
        ShaderProg::SetUni1i("depthOnly", 1);
    } else{
        glm::mat4 dLightSpaceVP = glm::ortho(-400.f, 400.f, -300.f, 300.f, NEAR_D, FAR_D) * Cam(glm::vec3(0.f, 5.f, 0.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), 0).LookAt(); //Ensure projection frustum size is correct so no fragments of objs are clipped (fragments of objs not in the depth/... map will not produce shadows)
        glm::mat4 sLightSpaceVP = glm::ortho(-400.f, 400.f, -300.f, 300.f, NEAR_S, FAR_S) * cam.LookAt(); //Non-linear depth due to perspective division with its noticeable range close to the near plane when visualising depth buffer
        basicShaderProg->Use();
        ShaderProg::SetUni1i("depthOnly", 0);
        ShaderProg::SetUni1i("showShadowsD", 1);
        ShaderProg::SetUni1i("showShadowsS", 1);
        ShaderProg::SetUniMtx4fv("dLightSpaceVP", glm::value_ptr(dLightSpaceVP));
        ShaderProg::SetUniMtx4fv("sLightSpaceVP", glm::value_ptr(sLightSpaceVP));
        for(short i = 0; i < 2; ++i){
            ShaderProg::UseTex(GL_TEXTURE_2D, depthTexs[i], ~i & 1 ? "shadowMapD" : "shadowMapS");
        }
    }

    if(showTerrainNormals){
        RenderTerrainNormals(cam);
    }
    basicShaderProg->Use();
    ShaderProg::SetUni1i("useFog", fogType > -1);
    ShaderProg::SetUni3f("fog.colour", .7f, .7f, .7f);
    ShaderProg::SetUni1f("fog.start", 100.f);
    ShaderProg::SetUni1f("fog.end", 2000.f);
    ShaderProg::SetUni1f("fog.density", .0005f);
    ShaderProg::SetUni1i("fog.type", fogType);

    ///Render terrain
    ShaderProg::SetUni1i("useMultiTex", 1);
    ShaderProg::UseTex(GL_TEXTURE_2D, meshes[3]->textures[1], "lowTex");
    ShaderProg::UseTex(GL_TEXTURE_2D, meshes[3]->textures[2], "highTex");
    SetUnis(cam, 2, glm::vec3(0.f, -100.f, 0.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(500.f, 100.f, 500.f));
    meshes[3]->textures[0].SetActiveOnMesh(1);
    meshes[3]->Render(GL_TRIANGLES);
    ShaderProg::StopUsingTex(GL_TEXTURE_2D, meshes[3]->textures[1]);
    ShaderProg::StopUsingTex(GL_TEXTURE_2D, meshes[3]->textures[2]);
    ShaderProg::SetUni1i("useMultiTex", 0);

    ///Render wave
    if(enCubemap){
        ShaderProg::SetUni1i("reflection", 1);
        ShaderProg::SetUni1i("wave", 1);
        ShaderProg::SetUni1f("time", (float)glfwGetTime());
        ShaderProg::UseTex(GL_TEXTURE_CUBE_MAP, *enCubemap, "cubemapSampler");
        SetUnis(cam, 2, glm::vec3(0.f, -50.f, 0.f), glm::vec4(1.f, 0.f, 0.f, 0.f), glm::vec3(230.f, 1.f, 170.f));
        meshes[4]->textures[0].SetActiveOnMesh(1);
        meshes[4]->Render(GL_TRIANGLES);
        ShaderProg::StopUsingTex(GL_TEXTURE_CUBE_MAP, *enCubemap);
        ShaderProg::SetUni1i("wave", 0);
        ShaderProg::SetUni1i("reflection", 0);
    }

    ///Render misc. stuff
    SetUnis(cam, 2, glm::vec3(-200.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, -200.f / 500.f, -10.f / 500.f), -10.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(5.f));
    models[11]->Render(GL_TRIANGLES, true);
    SetUnis(cam, 2, glm::vec3(150.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 150.f / 500.f, 180.f / 500.f) - 10.f, 180.f), glm::vec4(0.f, 1.f, 0.f, 180.f), glm::vec3(10.f));
    models[12]->Render(GL_TRIANGLES, true);

    ///Render sword
    ShaderProg::SetUni1i("useColourMultiplier", 1);
    ShaderProg::SetUni3f("colourMultiplier", glm::vec3(200.f));
    SetUnis(cam, 2, glm::vec3(-20.f, 20.f + sin(glfwGetTime()) * 5.f, -10.f), glm::vec4(0.f, 1.f, 0.f, 90.f), glm::vec3(5.f));
    models[13]->Render(GL_TRIANGLES, true);
    ShaderProg::SetUni1i("useColourMultiplier", 0);

    ///Render wolves
    SetUnis(cam, 2, glm::vec3(160.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 160.f / 500.f, -70.f / 500.f), -70.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(10.f));
    models[14]->Render(GL_TRIANGLES, true);
    SetUnis(cam, 2, glm::vec3(220.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 220.f / 500.f, -60.f / 500.f), -60.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(10.f));
    models[14]->Render(GL_TRIANGLES, true);
    SetUnis(cam, 2, glm::vec3(190.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 190.f / 500.f, -30.f / 500.f), -30.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(10.f));
    models[14]->Render(GL_TRIANGLES, true);
    SetUnis(cam, 2, glm::vec3(230.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 230.f / 500.f, 10.f / 500.f), 10.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(10.f));
    models[14]->Render(GL_TRIANGLES, true);
    SetUnis(cam, 2, glm::vec3(160.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 160.f / 500.f, 30.f / 500.f), 30.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(10.f));
    models[14]->Render(GL_TRIANGLES, true);
    SetUnis(cam, 2, glm::vec3(180.f, -100.f + 100.f * Mesh::ReadHeightMap(Mesh::heightMap, 180.f / 500.f, 40.f / 500.f), 40.f), glm::vec4(0.f, 1.f, 0.f, 0.f), glm::vec3(10.f));
    models[14]->Render(GL_TRIANGLES, true);
    ShaderProg::SetUni1i("bump", 0);

    RenderTreesAndRocks(cam);
    RenderSky(cam, 1); //Draw opaque objs first so depth buffer is filled with depth values of opaque objs and hence call frag shader to render only frags which pass the early depth test (saves bandwidth as frag shader does not run for frags that fail early depth test)

    if(!depthTexs){
        glCullFace(GL_BACK); //To prevent face culling of quads
    }
    const size_t& mySize = particleSystem.RetrieveEntityPool().size();
    std::vector<std::pair<std::tuple<glm::vec3, float, glm::vec3>, std::pair<glm::vec4, int>>> myVec(mySize);
    for(size_t i = 0; i < mySize; ++i){
        Entity* const& entity = particleSystem.RetrieveEntityPool()[i];
        if(entity->attribs.active){
            switch(entity->attribs.type){
                case Entity::EntityType::Rain:
                    myVec[i] = {
                        {
                            entity->attribs.pos,
                            atan2(cam.GetPos().x - entity->attribs.pos.x, cam.GetPos().z - entity->attribs.pos.z),
                            glm::vec3(entity->attribs.scale)
                        }, {entity->attribs.colour, -1}};
                    break;
                case Entity::EntityType::Smoke:
                    myVec[i] = {
                        {
                            entity->attribs.pos,
                            atan2(cam.GetPos().x - entity->attribs.pos.x, cam.GetPos().z - entity->attribs.pos.z),
                            glm::vec3(entity->attribs.scale)
                        }, {glm::vec4(glm::vec3(1.f - entity->attribs.life / 3.f), entity->attribs.life / 3.f), 0}};
                    break;
                case Entity::EntityType::Swirl:
                    myVec[i] = {
                        {
                            entity->attribs.pos,
                            atan2(cam.GetPos().x - entity->attribs.pos.x, cam.GetPos().z - entity->attribs.pos.z),
                            glm::vec3(entity->attribs.scale)
                        }, {entity->attribs.colour, -1}};
                    break;
            }
        }
    }

    ///Render particles through batch rendering
    basicShaderProg->Use();
    ShaderProg::SetUni1i("particle", 1);
    ShaderProg::UseTex(GL_TEXTURE_2D, meshes[1]->textures[2], "diffuseMaps[0]");
    SetUnis(cam, 2);
    meshes[5]->BatchRender(GL_TRIANGLES, myVec);
    ShaderProg::StopUsingTex(GL_TEXTURE_2D, meshes[1]->textures[2]);
    ShaderProg::SetUni1i("particle", 0);

    ///Render particles through instanced rendering
    SetUnis(cam, 2);
    ShaderProg::SetUni1i("useMat", 1);
    meshes[1]->textures[1].SetActiveOnMesh(1);
    meshes[1]->InstancedRender(GL_TRIANGLES, grassModelMats);
    ShaderProg::SetUni1i("useMat", 0);
    if(!depthTexs){
        glCullFace(GL_FRONT); //To prevent...
    }

    RenderCampfire(cam);
    ShaderProg::StopUsingAllTexs();
}

void Scene::BlurRender(const uint& brightTexRefID, const bool& horizontal){
    blurShaderProg->Use();
    ShaderProg::SetUni1i("horizontal", horizontal);
    ShaderProg::UseTex(GL_TEXTURE_2D, brightTexRefID, "texSampler");
    meshes[1]->Render(GL_TRIANGLES);
    ShaderProg::StopUsingTex(GL_TEXTURE_2D, brightTexRefID);
}

void Scene::DefaultRender(const uint& screenTexRefID, const uint& blurTexRefID, const int& typePPE, const bool& lineariseDepth, const glm::vec3& translate, const glm::vec3& scale) const{
    screenQuadShaderProg->Use();
    const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.f), translate), scale);
    ShaderProg::SetUniMtx4fv("model", &model[0][0], 0);
    ShaderProg::SetUni1i("screenQuad", 1);
    ShaderProg::SetUni1i("typePPE", typePPE);
    ShaderProg::UseTex(GL_TEXTURE_2D, screenTexRefID, "screenTex");
    ShaderProg::UseTex(GL_TEXTURE_2D, blurTexRefID, "blurredTex");
    if(lineariseDepth){
        ShaderProg::SetUni1i("lineariseDepth", 1);
        ShaderProg::SetUni1f("near", NEAR_D);
        ShaderProg::SetUni1f("far", FAR_D);
    }
    meshes[1]->Render(GL_TRIANGLES);
    ShaderProg::StopUsingTex(GL_TEXTURE_2D, screenTexRefID);
    ShaderProg::StopUsingTex(GL_TEXTURE_2D, blurTexRefID);
    ShaderProg::SetUni1i("screenQuad", 0);
    ShaderProg::SetUni1i("lineariseDepth", 0);
}