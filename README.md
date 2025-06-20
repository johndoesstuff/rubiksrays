# RubiksRays

A fully animated 3d Rubiks Cube in your terminal. Written in C++ with [FTXUI](https://github.com/ArthurSonzogni/FTXUI) and [GLM](https://github.com/g-truc/glm).

![RubiksRays Demo](RubiksRays.gif)

---

## Features

- Move history display
- Real-time 3d rendering using scanline triangle rasterization
- Keybinds for all standard Rubiks cube moves with animated transitions
- Move history compression (e.g. `LLL` -> `l`, `UUu` -> `U`)
- Togglable onscreen help
- Randomized scrambles and infinite undo history

---

## Controls

#### Camera
w/s Tilt up/down
a/d Rotate left/right

#### Face Moves
i/o U/U'
p/; R/R'
u/j L/L'
k/l F/F'
,/. B/B'
m// D/D'

#### Cube Rotations
r/f X/X'
q/e Y/Y'
x/c Z/Z'

#### Misc
space Random Move
z Undo Last Move
h Toggle Help
Ctrl+c Exit

## Build Instructions

### Dependencies

- A C++20 Compatible compiler
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)
- [GLM](https://github.com/g-truc/glm)
- `cmake` ≥ 3.28

### Build

```bash
git clone https://github.com/johndoesstuff/RubiksRays.git
cd RubiksRays
cmake -B build
cmake --build build
./build/RubiksRays
```

## Misc

Made by me as an introduction to programming in C++ (I usually prefer C). I'll likely make more programs in C++ in the future as I was pleasantly surprised with how convenient a lot of features were.
