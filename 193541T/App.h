#pragma once
#include "Scene.h"
#include "Buffer.h"

class App final{
	float lastFrame;
	float camResetBT;
	float cullBT;
	float polyModeBT;
	float ppeTypeBT;
	int typePPE;
	Scene* scene;
	Framebuffer* frontFBO;
	Framebuffer* dDepthMapFBO;
	Framebuffer* sDepthMapFBO;
	Framebuffer* enFBO;
	Framebuffer* intermediateFBO;
public:
	App();
	~App();
	static GLFWwindow* win;
	static bool Key(int);
	void Init(), Update(const Cam&);
	void Render(const Cam&) const;



	enum struct FBOType{
		Normal,
		PingPong0,
		PingPong1,
		Amt
	};
	enum struct TexType{
		Lit = 0,
		Bright,
		PingPong0,
		PingPong1,
		Amt
	};

	uint FBORefIDs[(int)FBOType::Amt];
	uint texRefIDs[(int)TexType::Amt];
	uint RBORefIDs[1];
};

template <class T>
inline T& operator++(T& myEnum){
	myEnum = T((int)myEnum + 1);
	return myEnum;
}