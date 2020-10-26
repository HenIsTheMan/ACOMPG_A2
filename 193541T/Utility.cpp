#include "Utility.h"

PointLight::PointLight(const glm::vec3& pos, const float& constant, const float& linear, const float& quadratic) noexcept{
    this->pos = pos;
    this->constant = constant;
    this->linear = linear;
    this->quadratic = quadratic;
}

DirectionalLight::DirectionalLight(const glm::vec3& dir) noexcept{
    this->dir = dir;
}

Spotlight::Spotlight(const glm::vec3& pos, const glm::vec3& dir, const float& cosInnerCutoff, const float& cosOuterCutoff) noexcept{
    this->pos = pos;
    this->dir = dir;
    this->cosInnerCutoff = cosInnerCutoff;
    this->cosOuterCutoff = cosOuterCutoff;
}