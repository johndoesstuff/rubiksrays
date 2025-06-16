#include <iostream>
#include <ftxui/screen/screen.hpp>

int main() {
	auto screen = ftxui::Screen::Create(
		ftxui::Dimension::Full(),
		ftxui::Dimension::Full()
	);

	auto& pixel = screen.PixelAt(10, 5);

	pixel.character = U'X';
	pixel.foreground_color = ftxui::Color::Red;
	pixel.bold = true;
	screen.Print();
}
