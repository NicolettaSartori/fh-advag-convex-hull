# ADVAG

# Start the compiled program
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
