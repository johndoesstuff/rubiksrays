#include "SDF.hpp"

double sdf_sphere(const glm::vec3& pos, const double r) {
	return glm::length(pos) - r;
}

double sdf_bounded_plane(const glm::vec3& pos, const glm::vec3& center, const glm::vec3& normal, const glm::vec2& size) {
	glm::vec3 rel = pos - center;

	float dist_to_plane = glm::dot(rel, normal);

	glm::vec3 on_plane = rel - normal * dist_to_plane;

	glm::vec3 u = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
	if (glm::length(u) < 1e-4f) u = glm::normalize(glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f)));
	glm::vec3 v = glm::cross(normal, u);

	float local_x = glm::dot(on_plane, u);
	float local_y = glm::dot(on_plane, v);

	float dx = std::max(std::abs(local_x) - size.x, 0.0f);
	float dy = std::max(std::abs(local_y) - size.y, 0.0f);

	return glm::length(glm::vec2(dx, dy)) + std::abs(dist_to_plane);
}

HitResult sdf_scene(const glm::vec3& pos, const std::vector<SDFObject>& objects) {
	double epsilon = 1e-5;
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
