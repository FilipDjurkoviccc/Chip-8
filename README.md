# CHIP-8 Emulator

A small CHIP-8 emulator written in C++ with OpenGL/GLFW for rendering.

The project is still a work in progress, but it can load and run basic CHIP-8
ROMs from the `roms/` folder. The emulator core is kept separate from the
window, input and rendering code so it is easier to extend later.

## Features

- CHIP-8 CPU cycle with memory, registers, stack, timers and keypad state
- ROM loading from disk
- 64x32 monochrome display
- OpenGL rendering through GLFW and GLAD
- Basic keyboard input mapping
- Sample ROMs included in `roms/`

## Requirements

- CMake
- A C++14 compiler
- GLFW 3
- OpenGL

GLAD is included in the repository under `includes/glad`.

## Build

From the project root:

```bash
cmake .
cmake --build .
```

This creates the `chip8` executable in the project root.

## Run

Pass a ROM path as the first argument:

```bash
./chip8 roms/PONG
```

Other included ROMs can be found in the `roms/` directory.

## Controls

CHIP-8 uses a 16-key hexadecimal keypad. It is mapped to the keyboard like
this:

```text
CHIP-8 keypad        Keyboard

1 2 3 C              1 2 3 4
4 5 6 D              Q W E R
7 8 9 E              A S D F
A 0 B F              Z X C V
```

Press `Esc` to close the emulator window.

## Project Structure

```text
src/
  Chip8.hpp       Emulator interface
  Chip8.cpp       CPU, memory, display and instruction logic
  main.cpp        Window, input, rendering and application loop
  shader.h        Small shader helper

shaders/
  vertexshader.vs
  fragmentshader.fs

roms/
  Sample CHIP-8 ROMs
```
