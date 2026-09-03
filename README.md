# RubiksRays

Animated 3d Rubiks Cube in your terminal... or on a fake terminal in your browser if that's your thing. Written in C++ with [FTXUI](https://github.com/ArthurSonzogni/FTXUI) and [GLM](https://github.com/g-truc/glm).

![RubiksRays Demo](RubiksRays.gif)

### [Try Me!](https://johndoesstuff.github.io/rubiksrays/)

## Features

- You can see your moves :P
- It is 3d
- The keybinds make sense
- Move history compression (e.g. `LLL` -> `l`, `UUu` -> `U`)
- Randomized scrambles and infinite undo history

## Controls

#### Camera
`w`/`s` Tilt up/down

`a`/`d` Rotate left/right

#### Face Moves
`i`/`o` U/U'

`p`/`;` R/R'

`u`/`j` L/L'

`k`/`l` F/F'

`,`/`.` B/B'

`m`/`/` D/D'

#### Cube Rotations
`r`/`f` X/X'

`q`/`e` Y/Y'

`x`/`c` Z/Z'


## Misc
`space` Random Move

`z` Undo Last Move

`h` Toggle Help

`^C` Exit

---

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

### Web (WebAssembly)

Run me on the web! (Idk why you would though)

```bash
./web/build.sh                      # needs emcc; set GLM=/path/to/glm if headers aren't in /usr/include/glm
python3 -m http.server -d web       # then open http://localhost:8000
```

## Misc

Made by me as an introduction to programming in C++ with web deployment assisted by Claude. I'll likely make more programs in C++ in the future as I was pleasantly surprised with how convenient a lot of features were.
