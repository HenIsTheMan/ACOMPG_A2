#include "App.h"
#include "Generic.h"
#include "Global.h"

GLFWwindow* App::win = 0; //Render window

App::App(): camResetBT(0.f), cullBT(0.f), polyModeBT(0.f), ppeTypeBT(0.f), typePPE(0){
    Init();
    scene = new Scene;
}

App::~App(){
    delete scene;
    frontFBO->Del();
    dDepthMapFBO->Del();
    sDepthMapFBO->Del();
    enFBO->Del();
    intermediateFBO->Del();
    glDeleteBuffers(sizeof(FBORefIDs) / sizeof(FBORefIDs[0]), FBORefIDs);
    glDeleteTextures(sizeof(texRefIDs) / sizeof(texRefIDs[0]), texRefIDs);
    glDeleteBuffers(sizeof(RBORefIDs) / sizeof(RBORefIDs[0]), RBORefIDs);
    ShaderProg::ClearShaderCache();
    glfwTerminate(); //Clean/Del all GLFW's resources that were allocated
}

bool App::Key(int key){
    return bool(glfwGetKey(win, key));
}

void App::Init(){
    InitGL(win);
    frontFBO = new Framebuffer(GL_TEXTURE_2D_MULTISAMPLE, 3, 800, 600, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    dDepthMapFBO = new Framebuffer(GL_TEXTURE_2D, 2, 2048, 2048, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_BORDER); //GL_CLAMP_TO_BORDER to reduce oversampling of depth/... map (light-space projected frag outside light's visible frustum > 1.f so sample depth/... map outside its [0.f, 1.f] range and makes currDepth > 1.f which causes area outside... to be in shadow) by making closestDepth of light-space projected frag outside... always 1.f
    sDepthMapFBO = new Framebuffer(GL_TEXTURE_2D, 2, 4096, 4096, GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_BORDER); //All frags outside... will have no shadows //Based on the texture's wrapping method, we will get incorrect depth results not based on the real depth values from the light source??
    enFBO = new Framebuffer(GL_TEXTURE_CUBE_MAP, 1, 1700, 1700, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
    intermediateFBO = new Framebuffer(GL_TEXTURE_2D, 0, 800, 600, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);



    glGenFramebuffers(sizeof(FBORefIDs) / sizeof(FBORefIDs[0]), FBORefIDs);
    glGenTextures(sizeof(texRefIDs) / sizeof(texRefIDs[0]), texRefIDs);
    glGenRenderbuffers(sizeof(RBORefIDs) / sizeof(RBORefIDs[0]), RBORefIDs);

    glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBOType::Normal]);
        for(TexType i = TexType::Lit; i <= TexType::Bright; ++i){
            int currTexRefID;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexRefID);
            glBindTexture(GL_TEXTURE_2D, texRefIDs[(int)i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(i) - int(TexType::Lit), GL_TEXTURE_2D, texRefIDs[(int)i], 0);
            glBindTexture(GL_TEXTURE_2D, currTexRefID);
        }

        glBindRenderbuffer(GL_RENDERBUFFER, RBORefIDs[0]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 2048, 2048);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBORefIDs[0]);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
            return (void)puts("myFBO is incomplete!\n");
        }
    for(FBOType i = FBOType::PingPong0; i <= FBOType::PingPong1; ++i){
        glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)i]);
        int currTexRefID;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currTexRefID);
        glBindTexture(GL_TEXTURE_2D, texRefIDs[int(TexType::PingPong0) + int(FBOType::PingPong1) - int(i)]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 2048, 2048, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texRefIDs[int(TexType::PingPong0) + int(FBOType::PingPong1) - int(i)], 0);
        glBindTexture(GL_TEXTURE_2D, currTexRefID);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void App::Update(const Cam& cam){
    float currFrame = (float)glfwGetTime();
    dt = currFrame - lastFrame;
    lastFrame = currFrame;

    scene->Update(cam);

    if(glfwGetKey(win, GLFW_KEY_R) && camResetBT <= currFrame){
        const_cast<Cam&>(cam).Reset();
        camResetBT = currFrame + .5f;
    }

    if(glfwGetKey(win, GLFW_KEY_1) && cullBT <= currFrame){
        glIsEnabled(GL_CULL_FACE) ? glDisable(GL_CULL_FACE) : glEnable(GL_CULL_FACE);
        cullBT = currFrame + .5f;
    }

    GLint polyMode;
    glGetIntegerv(GL_POLYGON_MODE, &polyMode);
    if(glfwGetKey(win, GLFW_KEY_2) && polyModeBT <= currFrame){
        glPolygonMode(GL_FRONT_AND_BACK, polyMode + (polyMode == GL_FILL ? -2 : 1));
        polyModeBT = currFrame + .5f;
    }

    if(glfwGetKey(win, GLFW_KEY_3) && ppeTypeBT <= currFrame){
        typePPE += (typePPE == 5 ? -5 : 1);
        ppeTypeBT = currFrame + .5f;
    }
}

void App::Render(const Cam& cam) const{
    ///1st...: render scenes from lights' POV to depth/shadow map (occluded frags [in shadow], sample closest depth value from depth/... map and check it against depth value of curr frag being processed in frag shader)
    glCullFace(GL_FRONT); //Solves peter panning (shadow mapping artefact, shadow bias applied to actual obj depth leads to visible offset of obj shadows, adjust shadow bias to avoid) for solid objs but causes shadows of plane objs to disappear //Take depth of back faces so shadows will form inside objs??

    glBindFramebuffer(GL_FRAMEBUFFER, dDepthMapFBO->GetRefID());
        glViewport(0, 0, 2048, 2048);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        scene->MyRender(Cam(glm::vec3(0.f, 5.f, 0.f), glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f), 0), nullptr, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, sDepthMapFBO->GetRefID());
        glViewport(0, 0, 4096, 4096);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        scene->MyRender(Cam(cam.GetPos(), cam.GetTarget(), cam.GetUp(), 1), nullptr, nullptr);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, 800, 600); //Render scene in win resolution

    glCullFace(GL_BACK); //Another consideration is that objects that are close to the shadow receiver (like the distant cube) may still give incorrect results??
    
    ////2nd set of render passes: render scenes with directional shadow mapping
    Tex depthTexs[]{dDepthMapFBO->GetTex(), sDepthMapFBO->GetTex()};

    ///Dynamic environment mapping (use dynamically-generated cubemap textured with 6 diff angles of scene as seen from a cam to create reflective and refractive surfaces that include other instances, avoid and/or use pre-rendered cubemaps as expensive)
    Cam enCam(glm::vec3(0.f, .2f, 0.f), glm::vec3(0.f), glm::vec3(0.f), 2, 1.f);
    float initialAngularFOV = angularFOV;
    angularFOV = 90.f;
    glViewport(0, 0, 1700, 1700);
    for(short i = 0; i < 6; ++i){
        switch(i){
            case 0: enCam.SetTarget(glm::vec3(1.f, 0.f, 0.f)); enCam.SetUp(glm::vec3(0.f, -1.f, 0.f)); break;
            case 1: enCam.SetTarget(glm::vec3(-1.f, 0.f, 0.f)); enCam.SetUp(glm::vec3(0.f, -1.f, 0.f)); break;
            case 2: enCam.SetTarget(glm::vec3(0.f, 1.f, 0.f)); enCam.SetUp(glm::vec3(0.f, 0.f, 1.f)); break;
            case 3: enCam.SetTarget(glm::vec3(0.f, -1.f, 0.f)); enCam.SetUp(glm::vec3(0.f, 0.f, 1.f)); break;
            case 4: enCam.SetTarget(glm::vec3(0.f, 0.f, 1.f)); enCam.SetUp(glm::vec3(0.f, -1.f, 0.f)); break;
            default: enCam.SetTarget(glm::vec3(0.f, 0.f, -1.f)); enCam.SetUp(glm::vec3(0.f, -1.f, 0.f));
        }
        glBindFramebuffer(GL_FRAMEBUFFER, enFBO->GetRefID());
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, enFBO->GetTex().GetRefID(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            scene->MyRender(enCam, nullptr, depthTexs);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glViewport(0, 0, 800, 600);
    angularFOV = initialAngularFOV;





    glViewport(0, 0, 2048, 2048);

    glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[(int)FBOType::Normal]);
    uint arr2[2]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(sizeof(arr2) / sizeof(arr2[0]), arr2);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    scene->MyRender(cam, &(enFBO->GetTex()), depthTexs);

    bool horizontal = true;
    const short amt = 50;
    for(short i = 0; i < amt; ++i){ //Blur... amt / 2 times horizontally and amt / 2 times vertically
        glBindFramebuffer(GL_FRAMEBUFFER, FBORefIDs[int(FBOType::PingPong0) + int(horizontal)]);
        scene->BlurRender(!i ? texRefIDs[(int)TexType::Bright] : texRefIDs[int(TexType::PingPong0) + int(horizontal)], horizontal);
        horizontal = !horizontal;
    }

    glViewport(0, 0, 800, 600);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(1.f, 0.82f, 0.86f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    scene->DefaultRender(texRefIDs[(int)TexType::Lit], texRefIDs[int(TexType::PingPong0) + int(!horizontal)], typePPE, 0, glm::vec3(0.f), glm::vec3(1.f));

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //scene->DefaultRender(dDepthMapFBO->GetTex().GetRefID(), dDepthMapFBO->GetTex().GetRefID(), typePPE, 0, glm::vec3(0.f), glm::vec3(1.f));
}

///Attach a z-buffer/depth buffer (stores depth value of frags clamped to [0, 1] from a cam's POV as 16, 24 or 32 bit floats, same width and height as colour buffer) and a stencil buffer as a single tex (each 32-bit value of the tex contains 24 bits of depth info and 8 bits of stencil info)
//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL); //Config the tex's formats to contain combined depth and stencil values
//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texColorBuffer, 0);

//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//glBlitFramebuffer(0, 0, 800, 600, 0, 0, 800, 600, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);