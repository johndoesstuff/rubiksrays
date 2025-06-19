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
	auto& pixel0 = screen.PixelAt(x0, y0);
	auto& pixel1 = screen.PixelAt(x1, y1);
	auto& pixel2 = screen.PixelAt(x2, y2);
	pixel0.foreground_color = triangle.color;
	pixel0.character = U'#';
	pixel1.foreground_color = triangle.color;
	pixel1.character = U'#';
	pixel2.foreground_color = triangle.color;
	pixel2.character = U'#';
}

int main() {
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

	while (true) {
		glm::vec3 camera(0.0, 0.0, -5.0);
		glm::vec3 camera_dir(0.0, 0.0, 1.0);
		glm::mat4 view = glm::lookAt(camera, camera + camera_dir, glm::vec3(0, 1, 0));

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto& pixel = screen.PixelAt(x, y);
				pixel.character = U' ';
			}
		}

		Triangle tri = {
			{
				glm::vec3(1.0, 1.0, 0.0),
				glm::vec3(1.0, -1.0, 0.0),
				glm::vec3(-1.0, 1.0, 0.0),
			},
			ftxui::Color::Red
		};

		float fov = glm::radians(70.0f);
		float aspectRatio = (float)width / (float)height / 2.0;
		glm::mat4 proj = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);

		render_triangle(tri, screen, proj, view);

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

