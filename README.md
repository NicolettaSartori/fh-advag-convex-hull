# ADVAG

# Build and run locally (Windows & Linux)

If SFML 3.1 is not installed on your system, CMake automatically downloads and builds SFML 3.1.0 via `FetchContent` (requires internet access on the first configure).

Configure into a top-level `build/` directory so local builds do not conflict with `/workspace/build` inside the Docker `dev` container.

## Windows

Requires a C++20 compiler (Visual Studio 2022/2026, MinGW-w64, or CLion) and CMake >= 3.24 (3.28+ for `FetchContent`).

- **CLion**: Open the project in CLion (configured for `src/CMakeLists.txt`) and build/run the `hull` target.
- **Command line (Ninja / MinGW)**:
  ```powershell
  cmake -S src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  .\build\hull.exe
  .\build\hull.exe --bench
  ```
- **Command line (Visual Studio / MSVC)**:
  ```powershell
  cmake -S src -B build
  cmake --build build --config Release
  .\build\Release\hull.exe
  .\build\Release\hull.exe --bench
  ```

## Linux

Requires a C++20 compiler, CMake >= 3.24 (3.28+ for `FetchContent`), Git, and X11/OpenGL/udev development headers for SFML (unless SFML 3.1 is already installed). On Debian/Ubuntu:

```bash
sudo apt-get install -y build-essential cmake ninja-build git \
  libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev
```

Build and run:

```bash
cmake -S src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/hull
./build/hull --bench
```

# Docker

## Start the compiled program
```bash
docker compose build app
```

Start the visualization:
```bash
docker compose run --rm --service-ports app
```

Once the terminal shows `Setting vertical sync not supported`, the app is running. Open in the browser:
```
http://localhost:6080/vnc.html
```

Benchmark (results in `src/protocol/<algorithm>.csv`):
```bash
docker compose run --rm app ./build/hull --bench
```

## Development in container
### First time
```bash
docker compose build
docker compose run --rm --service-ports dev

cmake -B build
cmake --build build && ./build/hull
```

### Dev
```bash
docker compose run --rm --service-ports dev
cmake --build build && ./build/hull
```

After every change (in the container):
```bash
cmake --build build && ./build/hull
```

### Browser
```
http://localhost:6080/vnc.html
```
