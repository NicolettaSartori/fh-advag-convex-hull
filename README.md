# ADVAG

# Setup
Terminal:
   ```bash
   docker compose build
   docker compose run --rm --service-ports dev bash -c "cmake -B build && cmake --build build && ./build/hull"
   ```

Once the terminal shows `Setting vertical sync not supported`, the app is running.

Browser:
   ```
   http://localhost:6080/vnc.html
   ```

## Run the existing build
Once the program has been built (see above), you can start it again without compiling:

Visualization:
   ```bash
   docker compose run --rm --service-ports dev ./build/hull
   ```

Benchmark (results in `src/protocol/<algorithm>.csv`):
   ```bash
   docker compose run --rm dev ./build/hull --bench
   ```


# Development
## First time
```bash
docker compose build
docker compose run --rm --service-ports dev

cmake -B build
cmake --build build && ./build/hull
```

## Dev
```bash
docker compose run --rm --service-ports dev
cmake --build build && ./build/hull
```

After every change (in the container):
```bash
cmake --build build && ./build/hull
```

## Browser
```
http://localhost:6080/vnc.html
```
