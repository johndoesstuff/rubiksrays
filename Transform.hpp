#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "CubeUnit.hpp"

struct Transform {
	std::vector<std::reference_wrapper<CubeUnit>> affected;
	double progress = 0.0f;
	glm::vec3 axis;
	float direction = 1.0f;
};
