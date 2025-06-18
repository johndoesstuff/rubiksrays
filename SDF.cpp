#include "SDF.hpp"

double sdf_sphere(const glm::vec3& pos, const double r) {
	return glm::length(pos) - r;
}

double sdf_bounded_plane(const glm::vec3& pos, const glm::vec3& center, const glm::vec3& normal, const glm::vec2& size) {
	glm::vec3 rel = pos - center;
	float d = glm::dot(rel, normal);
	glm::vec3 projected = rel - normal * d;

	glm::vec3 u = glm::normalize(glm::abs(normal.x) > 0.99f
			? glm::cross(normal, glm::vec3(0, 1, 0))
			: glm::cross(normal, glm::vec3(1, 0, 0)));
	glm::vec3 v = glm::cross(normal, u);

	float x = glm::dot(projected, u);
	float y = glm::dot(projected, v);

	float dx = std::max(std::abs(x) - size.x, 0.0f);
	float dy = std::max(std::abs(y) - size.y, 0.0f);

	float planar_dist = glm::length(glm::vec2(dx, dy));

	return std::sqrt(planar_dist * planar_dist + d * d);
}

HitResult sdf_scene(const glm::vec3& pos, const std::vector<SDFObject>& objects) {
	double epsilon = 1e-2;
	double min_dist = 1e12;
	int hit_id = -1;
	bool hit = false;

	for (const auto& obj : objects) {
		double sdf = obj.sdf(pos);
		if (sdf < min_dist) {
			min_dist = sdf;
			hit_id = obj.id;
		}
	}

	hit = min_dist < epsilon;

	return { min_dist, hit_id, hit };
}

const SDFObject* sdfobject_from_id(int id, const std::vector<SDFObject>& objects) {
	for (const auto& obj : objects) {
		if (obj.id == id) {
			return &obj;
		}
	}
	return nullptr;
}
