# ADVAG

# Setup
TODO

# Development
## First time
```bash
docker compose build
docker compose run --rm --service-ports dev

cmake -B build
cmake --build build && ./build/app
```

## Dev
```bash
docker compose run --rm --service-ports dev
cmake --build build && ./build/app
```

After every change (in the container):
```bash
cmake --build build && ./build/app
```

## Browser
```
http://localhost:6080/vnc.html
```
