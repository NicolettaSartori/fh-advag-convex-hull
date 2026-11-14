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

TODO build version

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
