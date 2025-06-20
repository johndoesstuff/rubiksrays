#include <iostream>
#include <csignal>
#include <ftxui/screen/screen.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "CubeUnit.hpp"
#include <termios.h>
#include <poll.h>
#include <chrono>
#include <thread>
#include "Transform.hpp"
#include <string>
#include <random>

// COLLECT USER INPUT
void set_raw_mode(bool enable) {
	static struct termios oldt;
	struct termios newt;
	if (enable) {
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	} else {
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	}
}

// DONT BREAK CURSOR ON EXIT
void handle_exit(int) {
	std::cout << "\033[?25h" << std::flush;
	set_raw_mode(false);
	std::exit(0);
}

// DETECT KEY EVENTS
char poll_keypress() {
	struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
	if (poll(&pfd, 1, 0) > 0) {
		char c;
		if (read(STDIN_FILENO, &c, 1) > 0) {
			return c;
		}
	}
	return '\0';
}

struct Vec2i {
	int x, y;
};

// RENDER LINE ON SCREEN WITH ZBUFFER FROM x0 x0 z0 TO x1 y1 z1
void render_line(
		int x0, int y0, float z0,
		int x1, int y1, float z1,
		ftxui::Screen& screen,
		ftxui::Color color,
		std::vector<std::vector<float>>& zbuffer
		) {
	bool steep = abs(y1 - y0) > abs(x1 - x0);

	if (steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
		std::swap(z0, z1);
	}

	int dx = x1 - x0;
	int dy = abs(y1 - y0);
	float dz = (dx == 0) ? 0.0f : (z1 - z0) / dx;
	float z = z0;

	int error = dx / 2;
	int ystep = (y0 < y1) ? 1 : -1;
	int y = y0;

	for (int x = x0; x <= x1; ++x) {
		int draw_x = steep ? y : x;
		int draw_y = steep ? x : y;

		if (draw_x >= 0 && draw_x < screen.dimx() && draw_y >= 0 && draw_y < screen.dimy() && z < zbuffer[draw_y][draw_x]) {
			zbuffer[draw_y][draw_x] = z;
			auto& pixel = screen.PixelAt(draw_x, draw_y);
			pixel.foreground_color = color;
			pixel.character = U'@';
		}

		z += dz;
		error -= dy;
		if (error < 0) {
			y += ystep;
			error += dx;
		}
	}
}

// SCANLINE TRIANGLE FILL FUNCTION
void fill_triangle(
		Vec2i v0, float z0,
		Vec2i v1, float z1,
		Vec2i v2, float z2,
		ftxui::Screen& screen, 
		ftxui::Color color, 
		std::vector<std::vector<float>>& zbuffer
		) {
	if (v0.y > v1.y) {
		std::swap(v0, v1);
		std::swap(z0, z1);
	}
	if (v0.y > v2.y) {
		std::swap(v0, v2);
		std::swap(z0, z2);
	}
	if (v1.y > v2.y) {
		std::swap(v1, v2);
		std::swap(z1, z2);
	}

	auto draw_scanline = [&](int y, int x_start, float z_start, int x_end, float z_end) {
		if (x_start > x_end) {
			std::swap(x_start, x_end);
			std::swap(z_start, z_end);
		}
		render_line(x_start, y, z_start, x_end, y, z_end, screen, color, zbuffer);
	};

	auto interpolate = [](int y0, int x0, int y1, int x1, int y) -> int {
		if (y1 == y0) return x0;
		return x0 + (x1 - x0) * (y - y0) / (y1 - y0);
	};

	auto interpolate_z = [](int y0, float z0, int y1, float z1, int y) -> float {
		if (y1 == y0) return z0;
		return z0 + (z1 - z0) * (y - y0) / (y1 - y0);
	};

	for (int y = v0.y; y <= v1.y; ++y) {
		int xa = interpolate(v0.y, v0.x, v2.y, v2.x, y);
		int xb = interpolate(v0.y, v0.x, v1.y, v1.x, y);
		float za = interpolate_z(v0.y, z0, v2.y, z2, y);
		float zb = interpolate_z(v0.y, z0, v1.y, z1, y);
		draw_scanline(y, xa, za, xb, zb);
	}

	for (int y = v1.y; y <= v2.y; ++y) {
		int xa = interpolate(v0.y, v0.x, v2.y, v2.x, y);
		int xb = interpolate(v1.y, v1.x, v2.y, v2.x, y);
		float za = interpolate_z(v0.y, z0, v2.y, z2, y);
		float zb = interpolate_z(v1.y, z1, v2.y, z2, y);
		draw_scanline(y, xa, za, xb, zb);
	}
}

// PROJECTS TRIANGLE VECTORS AND SENDS TO TRIANGLE SCANLINE FILL
void render_triangle(Triangle& triangle, ftxui::Screen& screen, glm::mat4 proj, glm::mat4 view, std::vector<std::vector<float>>& zbuffer) {
	glm::vec3 p0_view = glm::vec3(view * glm::vec4(triangle.points[0], 1.0f));
	glm::vec3 p1_view = glm::vec3(view * glm::vec4(triangle.points[1], 1.0f));
	glm::vec3 p2_view = glm::vec3(view * glm::vec4(triangle.points[2], 1.0f));

	glm::vec3 edge1 = p1_view - p0_view;
	glm::vec3 edge2 = p2_view - p0_view;
	glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
	
	float dot = glm::dot(normal, p0_view);

	if (dot <= 0) {
		return;
	}

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
	fill_triangle(vi0, v0.z, vi1, v1.z, vi2, v2.z, screen, triangle.color, zbuffer);
}

// CONSTRUCTS PLANE FROM NORMAL
Plane MakePlane(glm::vec3 normal, ftxui::Color color, bool invert_normal) {
	glm::vec3 p0 = {-0.5f, -0.5f, 0.0f};
	glm::vec3 p1 = { 0.5f, -0.5f, 0.0f};
	glm::vec3 p2 = { 0.5f,  0.5f, 0.0f};
	glm::vec3 p3 = {-0.5f,  0.5f, 0.0f};

	Triangle tri1 = {{p0, p1, p2}, color};
	if (invert_normal)
		tri1 = {{p2, p1, p0}, color};
	Triangle tri2 = {{p2, p3, p0}, color};
	if (invert_normal)
		tri2 = {{p0, p3, p2}, color};

	glm::vec3 axis = glm::cross(glm::vec3(0, 0, 1), normal);
	float angle = acos(glm::dot(glm::normalize(normal), glm::vec3(0, 0, 1)));

	glm::mat4 rotation = glm::mat4(1.0f);
	if (glm::length(axis) > 0.001f)
		rotation = glm::rotate(glm::mat4(1.0f), angle, glm::normalize(axis));

	return Plane{tri1, tri2, normal * 0.5f, rotation};
}

// CONSTRUCTS UNIT OF RUBIKS CUBE (27 TOTAL)
CubeUnit MakeCubeUnit(glm::vec3 position) {
	float padding = 0.2f;
	std::array<Plane, 6> faces = {
		MakePlane({ 0,  0,  1 + padding}, position.z < 1.0f ? ftxui::Color::Black : ftxui::Color::Red, true),    // Front
		MakePlane({ 0,  0, -1 - padding}, position.z > -1.0f ? ftxui::Color::Black : ftxui::Color::RedLight, false), // Back
		MakePlane({-1 - padding,  0,  0}, position.x > -1.0f ? ftxui::Color::Black : ftxui::Color::Blue, true),   // Left
		MakePlane({ 1 + padding,  0,  0}, position.x < 1.0f ? ftxui::Color::Black : ftxui::Color::Green, true),  // Right
		MakePlane({ 0,  1 + padding,  0}, position.y < 1.0f ? ftxui::Color::Black : ftxui::Color::YellowLight, true),  // Top
		MakePlane({ 0, -1 - padding,  0}, position.y > -1.0f ? ftxui::Color::Black : ftxui::Color::White, true), // Bottom
	};

	CubeUnit unit;
	for (int i = 0; i < 6; i++) {
		unit.plane[i] = faces[i];
	}
	unit.position = position;
	unit.rotation = glm::mat4(1.0f); // identity rotation
	return unit;
}

// APPLIES PLANAR TRANSFORMATIONS AND SENDS TO TRIANGLE RENDERING PIPELINE
void render_plane(const Plane& plane, ftxui::Screen& screen, const glm::mat4& proj, const glm::mat4& view, std::vector<std::vector<float>>& zbuffer) {
	glm::mat4 model = glm::translate(glm::mat4(1.0f), plane.position) * plane.rotation;

	for (const Triangle& tri : {plane.tri1, plane.tri2}) {
		Triangle transformed = tri;
		for (int i = 0; i < 3; i++) {
			glm::vec4 world = model * glm::vec4(tri.points[i], 1.0f);
			transformed.points[i] = glm::vec3(world);
		}
		render_triangle(transformed, screen, proj, view, zbuffer);
	}
}

// APPLIES CUBEUNIT TRANSFORMATIONS AND SENDS TO PLANE RENDERING PIPELINE
void render_cubeunit(CubeUnit cubeunit, ftxui::Screen& screen, glm::mat4 proj, glm::mat4 view, std::vector<std::vector<float>>& zbuffer) {
	for (int i = 0; i < 6; i++) {
		const Plane& plane = cubeunit.plane[i];

		Plane transformed_plane = plane;
		transformed_plane.position = glm::vec3(cubeunit.rotation * glm::vec4(plane.position, 1.0f));
		transformed_plane.rotation = cubeunit.rotation * plane.rotation;
		transformed_plane.position += cubeunit.position;

		render_plane(transformed_plane, screen, proj, view, zbuffer);
	}
}

// BUILDS CUBE (3x3x3)
Cube MakeCube() {
	float padding = 0.4f;
	Cube cube;
	for (int i = 0; i < 27; i++) {
		int x = i%3 - 1;
		int y = i/3%3 - 1;
		int z = i/9 - 1;
		CubeUnit cube_unit = MakeCubeUnit(glm::vec3((padding + 1.0f) * (float)x, (padding + 1.0f) * (float)y, (padding + 1.0f) * (float)z));
		cube.units[i] = cube_unit;
	}
	return cube;
}

int main() {
	// SETUP TERMINAL FOR VISUAL MODE AND GET USER INPUT
	set_raw_mode(true);
	static auto start_time = std::chrono::high_resolution_clock::now();
	std::signal(SIGINT, handle_exit);
	std::signal(SIGTERM, handle_exit);

	// INITIALIZE CLOCK (USEFUL FOR FPS AND DELTATIME TRANSITIONS)
	using Clock = std::chrono::high_resolution_clock;
	auto last_time = Clock::now();
	int fps = 0;
	const double target_framerate = 60.0;
	const auto target_frame_duration = std::chrono::duration<double>(1.0 / target_framerate);

	// INITIALIZE SCREEN
	auto screen = ftxui::Screen::Create(
		ftxui::Dimension::Full(),
		ftxui::Dimension::Full()
	);

	int width = screen.dimx();
	int height = screen.dimy();
	double aspect = width / (double)height / 2.0;

	// INITIALIZE ZBUFFER TO INFINITY
	std::vector<std::vector<float>> zbuffer(height, std::vector<float>(width, INFINITY));
	
	// INITIALIZE CUBE
	Cube cube = MakeCube();

	// INITIALIZE CAMERA CONTROLS
	float pitch = 0.0f;
	float yaw = 0.0f;
	float pitch_vel = 0.0f;
	float yaw_vel = 0.0f;

	// INITIALIZE TRANSFORM AND MOVE LIST
	Transform current_transform;
	current_transform.progress = 1.0f;
	std::vector<char> move_list = {};

	// MAIN LOOP
	while (true) {
		char key = poll_keypress();

		// CAMERA CONTROLS
		if (key == 'w') yaw_vel += 0.05;
		if (key == 'e') yaw_vel -= 0.05;
		if (key == 'o') pitch_vel += 0.05;
		if (key == 'p') pitch_vel -= 0.05;

		// STARTING A NEW CUBE MOVE CANCELS PREVIOUS ANIMATIONS
		bool cancel_transform = false;
		if (key == 'u') cancel_transform = true;
		if (key == 'd') cancel_transform = true;
		if (key == 'r') cancel_transform = true;
		if (key == 'l') cancel_transform = true;
		if (key == 'f') cancel_transform = true;
		if (key == 'b') cancel_transform = true;
		if (key == 'U') cancel_transform = true;
		if (key == 'D') cancel_transform = true;
		if (key == 'R') cancel_transform = true;
		if (key == 'L') cancel_transform = true;
		if (key == 'F') cancel_transform = true;
		if (key == 'B') cancel_transform = true;
		if (key == 'x') cancel_transform = true;
		if (key == 'y') cancel_transform = true;
		if (key == 'z') cancel_transform = true;
		if (key == 'X') cancel_transform = true;
		if (key == 'Y') cancel_transform = true;
		if (key == 'Z') cancel_transform = true;

		// SPACE = RANDOM MOVE
		if (key == ' ') {
			cancel_transform = true;
			std::string moves = "udrlfbUDRLFB";
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, moves.size() - 1);
			
			key = moves[dis(gen)];
		}

		// Q = UNDO LAST MOVE (DO INVERSE OF LAST MOVE)
		if (key == 'q') {
			cancel_transform = true;
			if (move_list.size() > 0) {
				int li = move_list.size() - 1;
				char op = move_list[li];
				if (tolower(op) == move_list[li]) {
					op = toupper(move_list[li]);
				} else {
					op = tolower(move_list[li]);
				}
				key = op;
			}
		}

		// CLAMP PRECISION TO AVOID ARTIFACTS
		if (current_transform.progress > 0.99) cancel_transform = true;
		if (std::abs(yaw_vel) < 0.001) yaw_vel = 0.0f;
		if (std::abs(pitch_vel) < 0.001) pitch_vel = 0.0f;

		// CALCULATE TRANSFORM PROGRESS
		double temp = current_transform.progress;
		current_transform.progress = current_transform.progress * 0.9 + 0.1;
		double delta_trans = current_transform.progress - temp;

		// CANCELING TRANSFORM SETS DELTA TRANSFORM TO THE REMAINING TRANSFORM
		if (cancel_transform) {
			delta_trans += 1.0 - current_transform.progress;
			current_transform.progress = 1.0f;
		}

		// APPLY TRANSFORM
		for ( auto& ref : current_transform.affected ) {
			CubeUnit& cube_unit = ref.get();

			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), (float)(current_transform.direction * delta_trans * glm::half_pi<float>()), current_transform.axis);
			cube_unit.position = glm::vec3(rot * glm::vec4(cube_unit.position, 1.0f));
			cube_unit.rotation = rot * cube_unit.rotation;
		}

		// KEY EVENTS
		if (key == 'u') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, -1, 0);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.y >= 1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'd') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 1, 0);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.y <= -1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'r') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(-1, 0, 0);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.x >= 1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'l') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(1, 0, 0);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.x <= -1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'f') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 0, -1);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.z >= 1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'b') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 0, 1);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.z <= -1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'U') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, -1, 0);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.y >= 1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'D') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 1, 0);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.y <= -1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'R') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(-1, 0, 0);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.x >= 1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'L') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(1, 0, 0);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.x <= -1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'F') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 0, -1);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.z >= 1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'B') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 0, 1);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				if (cube_unit.position.z <= -1.0) {
					current_transform.affected.push_back(std::ref(cube_unit));
				}
			}
		}
		if (key == 'x') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(-1, 0, 0);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				current_transform.affected.push_back(std::ref(cube_unit));
			}
		}
		if (key == 'y') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, -1, 0);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				current_transform.affected.push_back(std::ref(cube_unit));
			}
		}
		if (key == 'z') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 0, -1);
			current_transform.direction = 1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				current_transform.affected.push_back(std::ref(cube_unit));
			}
		}
		if (key == 'X') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(-1, 0, 0);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				current_transform.affected.push_back(std::ref(cube_unit));
			}
		}
		if (key == 'Y') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, -1, 0);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				current_transform.affected.push_back(std::ref(cube_unit));
			}
		}
		if (key == 'Z') {
			current_transform.affected = {};
			current_transform.progress = 0.0f;
			current_transform.axis = glm::vec3(0, 0, -1);
			current_transform.direction = -1.0f;
			for (int i = 0; i < 27; i++) {
				CubeUnit& cube_unit = cube.units[i];
				current_transform.affected.push_back(std::ref(cube_unit));
			}
		}

		// IF NEW TRANSFORM STARTED ADD CURRENT KEY TO MOVES
		if (current_transform.progress == 0.0f) {
			move_list.push_back(key);
		}

		// CHECK FOR CANCELING MOVES
		if (move_list.size() >= 2) {
			int li = move_list.size() - 1;
			int sli = li - 1;
			if (move_list[li] != move_list[sli] && tolower(move_list[li]) == tolower(move_list[sli])) {
				move_list.pop_back();
				move_list.pop_back();
			}
		}

		// CHECK FOR TRIPLE MOVES
		if (move_list.size() >= 3) { 
			int li = move_list.size() - 1;
			if (move_list[li] == move_list[li-1] && move_list[li-1] == move_list[li-2]) {
				// ANY TRIPLE MOVE CAN BE REDUCED TO ITS OPPOSITE; EX yyy => Y, YYY => y
				char op = move_list[li];
				if (tolower(op) == move_list[li]) {
					op = toupper(move_list[li]);
				} else {
					op = tolower(move_list[li]);
				}
				move_list.pop_back();
				move_list.pop_back();
				move_list.pop_back();
				move_list.push_back(op);
			}
		}

		// APPLY VELOCITIES FOR SMOOTH ROTATION
		yaw += yaw_vel;
		yaw_vel /= 1.1;
		pitch += pitch_vel;
		pitch_vel /= 1.1;

		// SMOOTHLY CLAMP VIEW TO PSEUDO-ISOMETRIC RANGE
		float pitch_iso = 0.6f;
		float yaw_iso = 0.6f;
		if (pitch > pitch_iso) {
			pitch = 0.99*pitch + 0.01*pitch_iso;
		} else if (pitch < -pitch_iso) {
			pitch = 0.99*pitch - 0.01*pitch_iso;
		}
		if (yaw > yaw_iso) {
			yaw = 0.99*yaw + 0.01*yaw_iso;
		} else if (yaw < -yaw_iso) {
			yaw = 0.99*yaw - 0.01*yaw_iso;
		}

		// PITCH CANNOT GO ABOVE OR BELOW 2PI
		pitch = glm::clamp(pitch, -glm::half_pi<float>() + 0.01f, glm::half_pi<float>() - 0.01f);

		// INITIALIZE CAMERA
		glm::vec3 camera;

		// R = CAMERA RADIUS TO CUBE
		float r = 8.0f;
		camera.x = r * std::cos(pitch) * std::sin(yaw);
		camera.y = r * std::sin(pitch);
		camera.z = r * std::cos(pitch) * std::cos(yaw);

		// SET VIEW AND ZBUFFER
		glm::mat4 view = glm::lookAt(camera, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		zbuffer.assign(height, std::vector<float>(width, INFINITY));

		// CLEAR SCREEN
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				auto& pixel = screen.PixelAt(x, y);
				pixel.character = U' ';
			}
		}

		// INITIALIZE PROJECTION MATRIX
		float fov = glm::radians(70.0f);
		float aspectRatio = (float)width / (float)height / 2.0;
		glm::mat4 proj = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);

		// RENDER CUBE UNITS
		for (int i = 0; i < 27; i++) {
			CubeUnit cube_unit = cube.units[i];
			render_cubeunit(cube_unit, screen, proj, view, zbuffer);
		}

		// DISPLAY FPS
		std::string fps_text = "FPS: " + std::to_string(fps);
		for (size_t i = 0; i < fps_text.size() && i < static_cast<size_t>(width); ++i) {
			auto& pixel = screen.PixelAt(i, 0); // top-left
			pixel.character = fps_text[i];
			pixel.foreground_color = ftxui::Color::White;
			pixel.bold = true;
		}

		// DISPLAY MOVE LIST
		for (size_t i = 0; i < move_list.size() && i < static_cast<size_t>(width); ++i) {
			auto& pixel = screen.PixelAt(i, height-1);
			pixel.character = move_list[i - ((move_list.size() > width) ? (width - move_list.size()) : 0)];
			pixel.foreground_color = ftxui::Color::White;
			pixel.bold = true;
		}

		// DISABLE CURSOR AND FLUSH SCREEN
		std::cout << "\033[?25l";
		std::cout << "\033[H";
		std::cout << screen.ToString();
		std::cout.flush();

		// PREPARE TIME FOR NEXT FRAME CALCULATIONS
		auto current_time = Clock::now();
		std::chrono::duration<double> delta = current_time - last_time;

		// CAP FRAMERATE TO 60FPS
		if (delta < target_frame_duration) {
			std::this_thread::sleep_for(target_frame_duration - delta);
		}
		current_time = Clock::now();
		delta = current_time - last_time;

		// UPDATE FPS
		fps = static_cast<int>(1.0 / delta.count());
		last_time = current_time;
	}
}

