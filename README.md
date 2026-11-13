# C++ + SFML in Docker

## Start
```bash
docker compose build          # Image einmalig bauen
docker compose run --rm --service-ports dev
```

Im Container:

```bash
cmake -B build
cmake --build build
./build/app
```

## Browser
```
http://localhost:6080/vnc.html
```
