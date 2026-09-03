// Minimal stand-in for FTXUI's Screen/Color used by the wasm build.
// Only the handful of members main.cpp touches are provided.
#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <algorithm>
#include <functional>

namespace ftxui {

enum class Color : uint8_t { Default, Black, Red, RedLight, Blue, Green, YellowLight, White };

struct Pixel {
	char character = ' ';
	Color foreground_color = Color::Default;
	bool bold = false;
};
static_assert(sizeof(Pixel) == 3, "JS reads pixels with a stride of 3 bytes");

struct Dimension { static Dimension Full() { return {}; } };

// Buffer lives at file scope so JS can read it after the frame returns.
inline int g_width = 80, g_height = 24;
inline std::vector<Pixel> g_pixels;

struct Screen {
	static Screen Create(Dimension, Dimension) {
		g_pixels.assign((size_t)g_width * g_height, Pixel{});
		return Screen{};
	}
	int dimx() const { return g_width; }
	int dimy() const { return g_height; }
	Pixel& PixelAt(int x, int y) { return g_pixels[(size_t)y * g_width + x]; }
};

} // namespace ftxui
