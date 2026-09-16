# Neon Runner

A fast-paced infinite runner game built with the Exgine Engine.

## Features

- **Procedural Obstacle Generation**: Random obstacles with varying heights and positions
- **Physics-Based Movement**: Realistic jumping with gravity and collision detection
- **Particle Effects**: Visual feedback for scoring and collisions
- **Dynamic Difficulty**: Speed increases over time, making the game progressively harder
- **Score System**: Track your current score and high score
- **Responsive Controls**: Keyboard (SPACE), mouse click, or touch input
- **Pause/Resume**: Press ESC to pause and resume the game
- **Visual Polish**: Neon aesthetic with smooth animations

## Controls

| Action | Input |
|--------|-------|
| Jump | SPACE / Left Click / Tap |
| Pause/Resume | ESC |
| Restart | R (when game over) |

## Building

### Prerequisites

- CMake 3.16 or higher
- C++17 compatible compiler (GCC 8+, Clang 7+, MSVC 2019+)
- Exgine Engine (parent directory or installed)
- OpenGL 3.3+ or Vulkan support

### Build Instructions

```bash
cd examples/game/neon_runner
mkdir build && cd build
cmake ..
cmake --build . --config Release
./neon_runner
```

## Gameplay

### Objective
Survive as long as possible by avoiding obstacles. The game speeds up over time.

### Scoring
- **Distance Points**: 10 points per second survived
- **Obstacle Bonus**: 100 points per obstacle passed
- **High Score**: Best score tracked during session

### Difficulty
- **Level 1**: Base speed (6.0 units/s)
- **Level 2+**: Speed +0.5 units/s every 10 seconds

## Customization

Edit constants in `main.cpp`:

```cpp
constexpr float INITIAL_OBSTACLE_SPEED = 6.0f;
constexpr float SPEED_INCREMENT = 0.5f;
constexpr float SPEED_INCREMENT_INTERVAL = 10.0f;
```

## License

See main Exgine repository LICENSE file.

---

**Enjoy the game!** 🎮
