#include <iostream>
#include <ftxui/screen/screen.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "SDF.hpp"
#include <csignal>
#include <omp.h>
#include <chrono>

void handle_exit(int) {
	std::cout << "\033[?25h" << std::flush;
	std::exit(0);
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
	
	int plane_id = 0;
	std::vector<SDFObject> scene;
	scene.push_back({
		[](const glm::vec3& pos) {
			return sdf_bounded_plane(pos, glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, 0.0, 1.0), glm::vec2(1.0, 1.0));
		},
		ftxui::Color::Green,
		plane_id++,
	});
	scene.push_back({
		[](const glm::vec3& pos) {
			return sdf_bounded_plane(pos, glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 0.0, -1.0), glm::vec2(1.0, 1.0));
		},
		ftxui::Color::Blue,
		plane_id++,
	});
	scene.push_back({
		[](const glm::vec3& pos) {
			return sdf_bounded_plane(pos, glm::vec3(1.0, 0.0, 0.0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(1.0, 1.0));
		},
		ftxui::Color::Red,
		plane_id++,
	});
	scene.push_back({
		[](const glm::vec3& pos) {
			return sdf_bounded_plane(pos, glm::vec3(-1.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec2(1.0, 1.0));
		},
		ftxui::Color::RedLight,
		plane_id++,
	});
	scene.push_back({
		[](const glm::vec3& pos) {
			return sdf_bounded_plane(pos, glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 1.0));
		},
		ftxui::Color::White,
		plane_id++,
	});
	scene.push_back({
		[](const glm::vec3& pos) {
			return sdf_bounded_plane(pos, glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, -1.0, 0.0), glm::vec2(1.0, 1.0));
		},
		ftxui::Color::YellowLight,
		plane_id++,
	});

	int max_steps = 20;
	float yaw_angle = 0.0f;
	float pitch_angle = 0.0f;

	while (true) {
		glm::vec3 camera(0.0, 0.0, -5.0);

		glm::mat4 yaw_rot = glm::rotate(glm::mat4(1.0f), yaw_angle, glm::vec3(0,1,0));
		glm::mat4 pitch_rot = glm::rotate(glm::mat4(1.0f), pitch_angle, glm::vec3(1,0,0));

		camera = pitch_rot * yaw_rot * glm::vec4(camera, 0.0);

		#pragma omp parallel for
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				glm::vec3 position = camera;
				glm::vec3 direction((x / (double)width - 0.5) * aspect, y / (double)height - 0.5, 0.5);
				direction = glm::normalize(direction);
				direction = pitch_rot * yaw_rot * glm::vec4(direction, 0.0);
				auto& pixel = screen.PixelAt(x, y);
				double total_dist = 0.0;
				pixel.character = U' ';
				for (int step = 0; step < max_steps; step++) {
					//double sdf = sdf_sphere(position, 3.0);
					HitResult result = sdf_scene(position, scene);
					double sdf = result.distance;
					if (sdf > 20) break;
					if (std::isnan(sdf)) break;
					position += direction * (float)sdf;
					total_dist += sdf;
					if (result.hit) {
						const SDFObject* hit_obj = sdfobject_from_id(result.id, scene);
						if (hit_obj != nullptr) {
							pixel.foreground_color = hit_obj->color;
							pixel.character = U'X';
							pixel.bold = true;
						}
						break;
					}
				}
			}
		}

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

		yaw_angle += 0.03f;
		pitch_angle += 0.01f;
	}
}
