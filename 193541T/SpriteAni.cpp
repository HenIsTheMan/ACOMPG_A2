#include "SpriteAni.h"

extern float dt;

SpriteAnimation::SpriteAnimation(int row, int col): row(row), col(col), currentTime(0), currentFrame(0), playCount(0), currentAnimation(""){}

SpriteAnimation::~SpriteAnimation(){
	for(auto iter = animationList.begin(); iter != animationList.end(); ++iter){
		if(iter->second){
			delete iter->second;
		}
	}
}

void SpriteAnimation::Update(){
	if(animationList[currentAnimation]->animActive){ //Check if the current animation is active
		currentTime += dt;
		int numFrame = (int)animationList[currentAnimation]->frames.size();
		float frameTime = animationList[currentAnimation]->animTime / numFrame;

		currentFrame = animationList[currentAnimation]->frames[std::min((int)animationList[currentAnimation]->frames.size() - 1, int(currentTime / frameTime))]; //Set curr frame based on curr time
		if(currentTime >= animationList[currentAnimation]->animTime){ //If curr time >= total animated time...
			if(playCount < animationList[currentAnimation]->repeatCount){
				///Increase count and repeat
				++playCount;
				currentTime = 0;
				currentFrame = animationList[currentAnimation]->frames[0];
			} else{ //If repeat count is 0 || play count == repeat count...
				animationList[currentAnimation]->animActive = false;
				animationList[currentAnimation]->ended = true;
			}
			if(animationList[currentAnimation]->repeatCount == -1){ //If ani is infinite...
				currentTime = 0.f;
				currentFrame = animationList[currentAnimation]->frames[0];
				animationList[currentAnimation]->animActive = true;
				animationList[currentAnimation]->ended = false;
			}
		}
	}
}

void SpriteAnimation::AddAnimation(std::string anim_name, int start, int end){
	Animation* anim = new Animation();
	for(int i = start; i < end; ++i){ //Ad in all the frames in the range
		anim->AddFrame(i);
	}
	animationList[anim_name] = anim; //Link anim to animation list
	if(currentAnimation == ""){ //Set the current animation if it does not exist
		currentAnimation = anim_name;
	}
	animationList[anim_name]->animActive = false;
}

void SpriteAnimation::AddSequeneAnimation(const str& name, const ::std::initializer_list<int>& frames){
	Animation* anim = new Animation();
	for(const int& frame: frames){
		anim->frames.emplace_back(frame);
	}
	animationList[name] = anim; //...
	if(currentAnimation == ""){ //...
		currentAnimation = name;
	}
	animationList[name]->animActive = false;
}

void SpriteAnimation::PlayAnimation(std::string anim_name, int repeat, float time){
	if(animationList[anim_name] != nullptr){ //Check if the anim name exist
		currentAnimation = anim_name;
		animationList[anim_name]->Set(repeat, time, true);
	}
}

void SpriteAnimation::Resume(){
	animationList[currentAnimation]->animActive = true;
}

void SpriteAnimation::Pause(){
	animationList[currentAnimation]->animActive = false;
}

void SpriteAnimation::Reset(){
	currentFrame = animationList[currentAnimation]->frames[0];
	playCount = 0;
}

SpriteAnimation* const SpriteAnimation::CreateSpriteAni(const unsigned& numRow, const unsigned& numCol){
	SpriteAnimation* mesh = new SpriteAnimation(numRow, numCol);
	mesh->indices = new std::vector<uint>;
	short myArr[6]{0, 1, 2, 0, 2, 3};

	float width = 1.f / numCol, height = 1.f / numRow;
	int offset = 0;
	for(unsigned i = 0; i < numRow; ++i){
		for(unsigned j = 0; j < numCol; ++j){
			float U = j * width;
			float V = 1.f - height - i * height;
			mesh->vertices.push_back({glm::vec3(-.5f, -.5f, 0.f), glm::vec4(1.f), glm::vec2(U, V), glm::vec3(0.f, 0.f, 1.f)});
			mesh->vertices.push_back({glm::vec3(.5f, -.5f, 0.f), glm::vec4(1.f), glm::vec2(U + width, V), glm::vec3(0.f, 0.f, 1.f)});
			mesh->vertices.push_back({glm::vec3(.5f, .5f, 0.f), glm::vec4(1.f), glm::vec2(U + width, V + height), glm::vec3(0.f, 0.f, 1.f)});
			mesh->vertices.push_back({glm::vec3(-.5f, .5f, 0.f), glm::vec4(1.f), glm::vec2(U, V + height), glm::vec3(0.f, 0.f, 1.f)});

			for(short k = 0; k < 6; ++k){
				(*(mesh->indices)).emplace_back(offset + myArr[k]);
			}
			offset += 4;
		}
	}
	return mesh;
}

void SpriteAnimation::Render(const int& primitive){
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
	glDrawElements(primitive, 6, GL_UNSIGNED_INT, (const void*)(long long(this->currentFrame) * 6 * sizeof(GLuint))); //What if no indices??
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