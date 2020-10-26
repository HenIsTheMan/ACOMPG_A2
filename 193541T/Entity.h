#pragma once
#include "Src.h"
#include "Mesh.h"

class Entity final{
	friend class EntityChief;
public:
	enum struct EntityType{
		Rain = 0,
		Smoke,
		Swirl,
		Amt
	};
	struct EntityAttribs final{
		EntityType type;
		bool active;
		float life;
		glm::vec4 colour;
		glm::vec3 scale;
		glm::vec3 collisionNormal;

		glm::vec3 pos;
		glm::vec3 vel;
		float mass;
		glm::vec3 force;

		glm::vec3 facingDir;
		float angularVel;
		float angularMass;
		glm::vec3 torque;
	};

	Entity();
	~Entity();

	///Getters
	const EntityAttribs& GetAttribs() const;
	Mesh* const& GetMesh() const;

	///Setters
	void SetAttribs(const EntityAttribs& attribs);
	void SetMesh(Mesh* const& mesh);

	EntityAttribs attribs;
private:
	Mesh* mesh;
};

class EntityChief final{
public:
	EntityChief();
	~EntityChief();

	Entity* const& FetchEntity();
	const std::vector<Entity*>& RetrieveEntityPool() const;
	void AddEntity();
private:
	std::vector<Entity*>* entityPool;
};