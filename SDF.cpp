#include "SDF.hpp"

double sdf_sphere(const glm::vec3& pos, const double r) {
	return glm::length(pos) - r;
}
