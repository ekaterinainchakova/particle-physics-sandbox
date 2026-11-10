# Particle Physics Sandbox

An interactive 2D physics simulation written in **C** with [raylib](https://www.raylib.com/) and compiled to **WebAssembly**, so it runs in the browser. Created as a university project for the *Principles of Programming* module (BSc Computer Science, UWE Bristol, 2026).

![Screenshot of the Particle Physics Sandbox](assets/screenshot.png)

## What it does

- Spawns coloured balls of random size; each ball has a **radius and a mass**.
- Supports up to **120 balls** at a **60 FPS** target (1000 x 700 window).
- **Ball-to-ball collisions:** overlap is corrected in proportion to mass, and an impulse is applied along the collision normal with a restitution of 0.82, so heavier balls push lighter ones.
- **Wall and floor bounces** with damping, plus a little floor friction.
- **Gravity well:** hold the right mouse button and every ball is pulled towards the cursor.
- **Adjustable gravity** and a live **debug overlay** (ball count, gravity, FPS).

## Controls

| Input | Action |
|---|---|
| Left click | Spawn a burst of balls at the cursor |
| Right click (hold) | Gravity well at the cursor |
| `Space` | Pause / resume |
| `R` | Reset the simulation |
| `C` | Clear all balls |
| `D` | Toggle the debug overlay |
| `Up` / `Down` | Increase / decrease gravity |

## Run it

The compiled web version is in the [`docs/`](docs/) folder. WebAssembly needs to be served over HTTP, so run:

```bash
cd docs
python3 -m http.server 8000
```

then open <http://localhost:8000>.

If GitHub Pages is enabled for this repository (Settings > Pages > Branch `main`, folder `/docs`), the demo is also available online.

## Build it

The project was built in the UWE Bristol module environment, which provides raylib for WebAssembly and a build script:

```bash
/opt/pop/bin/build-wasm.sh src/main.c
```

That script is specific to the UWE environment, so it will not exist on other machines. To build elsewhere you need raylib (see the [raylib web guide](https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5))); the source in `src/main.c` only depends on `raylib.h` and the C standard library (`math.h`, `stdbool.h`).

## How it works

Each frame the simulation updates every ball (gravity, optional gravity well, air drag, wall collisions) and then checks every pair of balls for collisions. The pairwise check is O(n^2), which is fine for 120 balls; a spatial grid would be the next step for much larger numbers.

## Project structure

```
src/main.c        simulation source code
shell.html        page template used for the web build
docs/             compiled WebAssembly build (open via a local server)
assets/           screenshot
resources/        logo images used by the page template
```

## Learning resources

W3Schools C tutorial, Learn-C.org, freeCodeCamp C videos and the module lecture materials.
