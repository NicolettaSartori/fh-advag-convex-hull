# Drei Stufen:
#   base  gemeinsame Grundlage: Compiler, SFML, virtueller Bildschirm + noVNC
#   dev   Entwicklung: der Code wird von außen nach /workspace gemountet
#   app   fertiges Programm: der Code wird ins Image kopiert und kompiliert
FROM ubuntu:26.04 AS base

ENV DEBIAN_FRONTEND=noninteractive \
    DISPLAY=:1 \
    SCREEN_RESOLUTION=1280x800x24

# SFML-Version, die aus dem Quellcode gebaut wird (Ubuntu liefert nur eine ältere).
ARG SFML_VERSION=3.1.0

RUN apt-get update && apt-get install -y --no-install-recommends \
        # Compiler & Build-Tools
        build-essential g++ gdb cmake ninja-build pkg-config git \
        # Abhängigkeiten, um SFML zu bauen
        libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev \
        libfreetype-dev libharfbuzz-dev libflac-dev libvorbis-dev libogg-dev \
        # Virtueller Bildschirm + Anzeige im Browser
        xvfb x11vnc fluxbox novnc websockify \
        # OpenGL per Software-Rendering (für SFML nötig)
        libgl1-mesa-dri mesa-utils \
        fonts-dejavu-core ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# SFML bauen und nach /usr/local installieren
RUN git clone --depth 1 --branch ${SFML_VERSION} https://github.com/SFML/SFML.git /tmp/sfml \
    && cmake -S /tmp/sfml -B /tmp/sfml/build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DSFML_USE_SYSTEM_DEPS=ON \
        -DSFML_BUILD_NETWORK=OFF \
    && cmake --build /tmp/sfml/build \
    && cmake --install /tmp/sfml/build \
    && ldconfig \
    && rm -rf /tmp/sfml

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN sed -i 's/\r$//' /usr/local/bin/entrypoint.sh && chmod +x /usr/local/bin/entrypoint.sh

EXPOSE 6080
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]

# ---- Entwicklung --------------------------------------------------------------
FROM base AS dev
WORKDIR /workspace
CMD ["bash"]

# ---- Fertiges Programm --------------------------------------------------------
FROM base AS app
ENV HULL_APP=1
WORKDIR /app
COPY src/ /app/
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
CMD ["./build/hull"]
