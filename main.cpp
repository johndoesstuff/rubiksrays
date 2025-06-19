#include <iostream>
#include <ftxui/screen/screen.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include "SDF.hpp"
#include "CubeUnit.hpp"
#include <csignal>
#include <omp.h>
#include <chrono>

void handle_exit(int) {
	std::cout << "\033[?25h" << std::flush;
	std::exit(0);
}

struct Vec2i {
	int x, y;
};

void render_line(int x0, int y0, int x1, int y1, ftxui::Screen& screen, ftxui::Color color) {
	bool steep = abs(y1 - y0) > abs(x1 - x0);

	if (steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = abs(y1 - y0);
	int error = dx / 2;
	int ystep = (y0 < y1) ? 1 : -1;
	int y = y0;

	for (int x = x0; x <= x1; ++x) {
		int draw_x = steep ? y : x;
		int draw_y = steep ? x : y;

		if (draw_x >= 0 && draw_x < screen.dimx() && draw_y >= 0 && draw_y < screen.dimy()) {
			auto& pixel = screen.PixelAt(draw_x, draw_y);
			pixel.foreground_color = color;
			pixel.character = U'#';
		}

		error -= dy;
		if (error < 0) {
			y += ystep;
			error += dx;
		}
	}
}

void fill_triangle(Vec2i v0, Vec2i v1, Vec2i v2, ftxui::Screen& screen, ftxui::Color color) {
	if (v0.y > v1.y) std::swap(v0, v1);
	if (v0.y > v2.y) std::swap(v0, v2);
	if (v1.y > v2.y) std::swap(v1, v2);

	auto draw_scanline = [&](int y, int x_start, int x_end) {
		if (x_start > x_end) std::swap(x_start, x_end);
		render_line(x_start, y, x_end, y, screen, color);
	};

	auto interpolate = [](int y0, int x0, int y1, int x1, int y) -> int {
		if (y1 == y0) return x0;
		return x0 + (x1 - x0) * (y - y0) / (y1 - y0);
	};

	for (int y = v0.y; y <= v1.y; ++y) {
		int xa = interpolate(v0.y, v0.x, v2.y, v2.x, y);
		int xb = interpolate(v0.y, v0.x, v1.y, v1.x, y);
		draw_scanline(y, xa, xb);
	}

	for (int y = v1.y; y <= v2.y; ++y) {
		int xa = interpolate(v0.y, v0.x, v2.y, v2.x, y);
		int xb = interpolate(v1.y, v1.x, v2.y, v2.x, y);
		draw_scanline(y, xa, xb);
	}
}

void render_triangle(Triangle& triangle, ftxui::Screen& screen, glm::mat4 proj, glm::mat4 view) {
	glm::vec4 v0 = proj * view * glm::vec4(triangle.points[0], 1.0f);
	glm::vec4 v1 = proj * view * glm::vec4(triangle.points[1], 1.0f);
	glm::vec4 v2 = proj * view * glm::vec4(triangle.points[2], 1.0f);

	int width = screen.dimx();
	int height = screen.dimy();

	v0 /= v0.w;
	v1 /= v1.w;
	v2 /= v2.w;

	int x0 = (int)((v0.x + 1.0f) * 0.5f * width);
	int y0 = (int)((1.0f - v0.y) * 0.5f * height);
	int x1 = (int)((v1.x + 1.0f) * 0.5f * width);
	int y1 = (int)((1.0f - v1.y) * 0.5f * height);
	int x2 = (int)((v2.x + 1.0f) * 0.5f * width);
	int y2 = (int)((1.0f - v2.y) * 0.5f * height);
	Vec2i vi0 = { x0, y0 };
	Vec2i vi1 = { x1, y1 };
	Vec2i vi2 = { x2, y2 };
	fill_triangle(vi0, vi1, vi2, screen, triangle.color);
}

Plane MakePlane(glm::vec3 normal, ftxui::Color color) {
	glm::vec3 p0 = {-0.5f, -0.5f, 0.0f};
	glm::vec3 p1 = { 0.5f, -0.5f, 0.0f};
	glm::vec3 p2 = { 0.5f,  0.5f, 0.0f};
	glm::vec3 p3 = {-0.5f,  0.5f, 0.0f};

	Triangle tri1 = {{p0, p1, p2}, color};
	Triangle tri2 = {{p2, p3, p0}, color};

	glm::vec3 axis = glm::cross(glm::vec3(0, 0, 1), normal);
	float angle = acos(glm::dot(glm::normalize(normal), glm::vec3(0, 0, 1)));

	glm::mat4 rotation = glm::mat4(1.0f);
	if (glm::length(axis) > 0.001f)
		rotation = glm::rotate(glm::mat4(1.0f), angle, glm::normalize(axis));

	return Plane{tri1, tri2, normal * 0.5f, rotation};
}

CubeUnit MakeCubeUnit(glm::vec3 position) {
	std::array<Plane, 6> faces = {
		MakePlane({ 0,  0,  1}, ftxui::Color::Red),    // Front
		MakePlane({ 0,  0, -1}, ftxui::Color::RedLight), // Back
		MakePlane({-1,  0,  0}, ftxui::Color::Blue),   // Left
		MakePlane({ 1,  0,  0}, ftxui::Color::Green),  // Right
		MakePlane({ 0,  1,  0}, ftxui::Color::White),  // Top
		MakePlane({ 0, -1,  0}, ftxui::Color::Yellow), // Bottom
	};

	CubeUnit unit;
	for (int i = 0; i < 6; i++) {
		unit.plane[i] = faces[i];
	}
	unit.position = position;
	unit.rotation = glm::mat4(1.0f); // identity rotation
	return unit;
}

void render_plane(const Plane& plane, ftxui::Screen& screen, const glm::mat4& proj, const glm::mat4& view) {
	glm::mat4 model = glm::translate(glm::mat4(1.0f), plane.position) * plane.rotation;

	for (const Triangle& tri : {plane.tri1, plane.tri2}) {
		Triangle transformed = tri;
		for (int i = 0; i < 3; i++) {
			glm::vec4 world = model * glm::vec4(tri.points[i], 1.0f);
			transformed.points[i] = glm::vec3(world);
		}
		render_triangle(transformed, screen, proj, view);
	}
}

void render_cubeunit(CubeUnit cubeunit, ftxui::Screen& screen, glm::mat4 proj, glm::mat4 view) {
	for (int i = 0; i < 6; i++) {
		Plane plane = cubeunit.plane[i];
		plane.position += cubeunit.position;
		plane.rotation *= cubeunit.rotation;
		render_plane(plane, screen, proj, view);
	}
}

int main() {
	static auto start_time = std::chrono::high_resolution_clock::now();
	std::signal(SIGINT, handle_exit);
	std::signal(SIGTERM, handle_exit);

	using Clock = std::chrono::high_resolution_clock;
	auto last_time = Clock::now();
	int fps = 0;

	auto screen = ftxui::Screen::Create(
		ftxui::Dimension::Full(),
		ftxui::Dimension::Full()
	);

	int width = screen.dimx();
	int height = screen.dimy();
	double aspect = width / (double)height / 2.0;
	
	int max_steps = 20;

	CubeUnit cube = MakeCubeUnit(glm::vec3(0.0f, 0.0f, 0.0f));

	while (true) {
		auto now = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float>(now - start_time).count();
		glm::vec3 camera(5.0f*std::sin(time), 0.0f, 5.0f*std::cos(time));
		glm::vec3 camera_dir = normalize(-camera);
		glm::mat4 view = glm::lookAt(camera, camera + camera_dir, glm::vec3(0, 1, 0));

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto& pixel = screen.PixelAt(x, y);
				pixel.character = U' ';
			}
		}

		float fov = glm::radians(70.0f);
		float aspectRatio = (float)width / (float)height / 2.0;
		glm::mat4 proj = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);

		render_cubeunit(cube, screen, proj, view);

		std::string fps_text = "FPS: " + std::to_string(fps);
		for (size_t i = 0; i < fps_text.size() && i < static_cast<size_t>(width); ++i) {
			auto& pixel = screen.PixelAt(i, 0); // top-left
			pixel.character = fps_text[i];
			pixel.foreground_color = ftxui::Color::White;
			pixel.bold = true;
		}

		std::cout << "\033[?25l";
		std::cout << "\033[H";
		std::cout << screen.ToString();
		std::cout.flush();

		auto current_time = Clock::now();
		std::chrono::duration<double> delta = current_time - last_time;
		last_time = current_time;
		fps = static_cast<int>(1.0 / delta.count());
	}
}

