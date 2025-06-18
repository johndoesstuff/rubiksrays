#pragma once

struct Triangle {
	glm::vec3 points[3];
	ftxui::Color color;
};

struct Plane {
	Triangle tri1;
	Triangle tri2;
	glm::vec3 position;
	glm::mat4 rotation;
};

struct CubeUnit {
	Plane plane[6];
	glm::vec3 position;
	glm::mat4 rotation;
};
