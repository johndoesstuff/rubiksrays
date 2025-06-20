//DEPRECATED

#pragma once

#include <glm/glm.hpp>
#include <ftxui/screen/screen.hpp>
#include <functional>
#include <optional>

struct HitResult {
	double distance;
	int id;
	bool hit;
};

struct SDFObject {
	std::function<double(const glm::vec3&)> sdf;
	ftxui::Color color;
	int id;
};

double sdf_sphere(const glm::vec3& pos, const double r);

double sdf_bounded_plane(const glm::vec3& pos, const glm::vec3& center, const glm::vec3& normal, const glm::vec2& size);

HitResult sdf_scene(const glm::vec3& pos, const std::vector<SDFObject>& objects);

const SDFObject* sdfobject_from_id(int id, const std::vector<SDFObject>& objects);
