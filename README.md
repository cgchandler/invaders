# C64 Invaders Game
**A full-featured Space Invaders inspired game for the Commodore 64, written in C using the Oscar64 compiler.**

## Overview
This project is a Space Invaders inspired game built for the Commodore 64 in C, compiled using the [Oscar64 compiler](https://github.com/drmortalwombat/oscar64).

The game runs on:
* Real Commodore 64 hardware or FPGA C64 emulators
* [VICE](https://vice-emu.sourceforge.io/) or other C64 emulators

## Screenshots
![Intro Screen](screenshot_intro.jpg "Intro Screen")

![Game Play](screenshot_game_play.jpg "Game Play")

## Features
* Intro Screen
* Attract Mode
* Animated Starfield Background
* SID Sound Effects
* Joystick and Keyboard Input Support
* Progressive Level Difficulty
* Classic Space-Invaders-Style Alien Formations
* Score and High Score Display
* Player Lives System
* Raster Timed Rendering
* Hardware Sprites
* Custom Character Set

### Controls
* Joystick in **Port 2**
* Keyboard controls using **A/D** and **Left/Right Cursor**
* Joystick button or space bar to fire weapon

### Gameplay
* Alien swarm attacks planet
* Aliens kill player by collision or a bomb launched during attack
* Player protects planet with missile launches
* Destructible player bases provide cover
* Alien speed increases as swarm size decreases
* Game ends when aliens reach ground or all reserve players are lost

### Display \& HUD
* Current Score
* High Score
* Level #
* Reserve Player Ships

## Build \& Run

### Requirements
* Oscar64 compiler - https://github.com/drmortalwombat/oscar64
* A C64 emulator (recommended: VICE) or real hardware

### Build Steps
* from the command line "make.bat"

## Play Online
[Play Invaders online running in Vice.js](https://www.cehost.com/invaders/)

## License

This project is licensed under the MIT License.

You are free to use, modify, and distribute this software for personal or commercial purposes, provided that the original copyright notice and license text are included with any substantial portions of the code.

See the [LICENSE](LICENSE) file for full details.
