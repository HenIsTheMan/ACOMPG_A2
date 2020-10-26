#include "Entity.h"

Entity::Entity():
	attribs({
		EntityType::Amt,
		false,
		0.f,
		glm::vec4(0.f),
		glm::vec3(0.f),
		glm::vec3(0.f),

		glm::vec3(0.f),
		glm::vec3(0.f),
		0.f,
		glm::vec3(0.f),

		glm::vec3(0.f),
		0.f,
		0.f,
		glm::vec3(0.f)
	}),
	mesh(nullptr)
{
}

Entity::~Entity(){
	if(mesh){
		delete mesh;
		mesh = nullptr;
	}
}

const Entity::EntityAttribs& Entity::GetAttribs() const{
	return attribs;
}

Mesh* const& Entity::GetMesh() const{
	return mesh;
}

void Entity::SetAttribs(const EntityAttribs& attribs){
	this->attribs = attribs;
}

void Entity::SetMesh(Mesh* const& mesh){
	this->mesh = mesh;
}

EntityChief::EntityChief():
	entityPool(new std::vector<Entity*>())
{
}

EntityChief::~EntityChief(){
	if(entityPool){
		for(Entity*& entity: *entityPool){
			delete entity;
			entity = nullptr;
		}
		delete entityPool;
	}
}

Entity* const& EntityChief::FetchEntity(){
	for(Entity* const& entity: *entityPool){
		if(!entity->attribs.active){
			return entity;
		}
	}
	AddEntity();
	(void)puts("1 entity was added to entityPool!\n");
	return entityPool->back();
}

const std::vector<Entity*>& EntityChief::RetrieveEntityPool() const{
	return *entityPool;
}

void EntityChief::AddEntity(){
	entityPool->emplace_back(new Entity());
}