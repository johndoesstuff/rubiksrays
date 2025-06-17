cmake_minimum_required(VERSION 3.28)
project(RubiksRays)
include(FetchContent)


FetchContent_Declare(ftxui
	GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
	GIT_TAG v6.1.9
)

FetchContent_MakeAvailable(ftxui)

add_executable(RubiksRays main.cpp)
target_link_libraries(RubiksRays
	PRIVATE ftxui::screen
	PRIVATE ftxui::dom
	PRIVATE ftxui::component
)
