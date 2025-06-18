#include <iostream>
#include <ftxui/screen/screen.hpp>
#include "SDF.hpp"

int main() {
	auto screen = ftxui::Screen::Create(
		ftxui::Dimension::Full(),
		ftxui::Dimension::Full()
	);

	int width = screen.dimx();
	int height = screen.dimy();
	double aspect = width / (double)height / 2.0;
	
	int max_steps = 50;
	double epsilon = 1e-5;
	glm::vec3 camera(0.0, 0.0, -5.0);

	std::cout << "Screen Width: " << width << " Height: " << height << std::endl;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			glm::vec3 position = camera;
			glm::vec3 direction((x / (double)width - 0.5) * aspect, y / (double)height - 0.5, 0.5);
			direction = glm::normalize(direction);
			auto& pixel = screen.PixelAt(x, y);
			double total_dist = 0.0;
			bool hit = false;
			for (int step = 0; step < max_steps; step++) {
				double sdf = sdf_sphere(position, 3.0);
				position += direction * (float)sdf;
				total_dist += sdf;
				if (sdf < epsilon) {
					hit = true;
					break;
				}
			}
			if (hit) pixel.character = U'X';
			pixel.foreground_color = ftxui::Color::Red;
			pixel.bold = true;
		}
	}
	screen.Print();

}
