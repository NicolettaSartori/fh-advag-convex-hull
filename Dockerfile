FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    DISPLAY=:1 \
    SCREEN_RESOLUTION=1280x800x24

RUN apt-get update && apt-get install -y --no-install-recommends \
        # Compiler & Build-Tools
        build-essential g++ gdb cmake ninja-build pkg-config git \
        # SFML (Version 2.6)
        libsfml-dev \
        # Virtueller Bildschirm + Anzeige im Browser
        xvfb x11vnc fluxbox novnc websockify \
        # OpenGL per Software-Rendering (für SFML nötig)
        libgl1-mesa-dri mesa-utils \
        fonts-dejavu-core ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

WORKDIR /workspace
EXPOSE 6080

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["bash"]
