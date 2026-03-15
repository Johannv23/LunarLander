# Lunar Lander – CS-UY 3113

**Author:** Johann Varghese  
**Course:** CS-UY 3113  
**Assignment:** Lunar Lander  
**Date Due:** 03/14/2026

---

## Description

This project is a Lunar Lander game built using Raylib in C++.

The player controls an astronaut lander and must safely land on one of two platforms
while managing fuel and avoiding the rocky terrain below.

The game features:

- Gravity-based physics with acceleration and velocity
- Thrust mechanics using acceleration (not direct velocity)
- Drift/deceleration when thrust keys are released
- Fuel system that depletes when thrusting
- Static safe landing platform (WIN)
- Moving platform that travels left and right (WIN)
- Rocky terrain that ends the game on contact (LOSE)
- Animated engine flame sprite sheet
- Win and lose end screens
- Fuel bar UI with color indicator

---

## Controls

| Key | Action |
|-----|--------|
| Up / W | Thrust upward |
| Left / A | Thrust left |
| Right / D | Thrust right |
| Close Window | Exit Game |

---

## Features Implemented

- Fixed timestep physics loop
- Gravity and acceleration-based movement
- Fuel mechanic with UI bar (green → yellow → red)
- AABB collision detection with overlap correction
- Moving platform with left/right bounce pattern
- Animated flame sprite sheet (4 frames)
- Win/Lose detection and end screens
- Delta time for all movement

---

## How to Run

In the project directory:
```bash
make
./raylib_app
```