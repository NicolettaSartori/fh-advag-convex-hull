#!/bin/bash
# Startet einen virtuellen Bildschirm und macht ihn unter http://localhost:6080 erreichbar.
set -e

Xvfb "$DISPLAY" -screen 0 "$SCREEN_RESOLUTION" -nolisten tcp > /tmp/xvfb.log 2>&1 &
sleep 1
fluxbox > /tmp/fluxbox.log 2>&1 &
x11vnc -display "$DISPLAY" -forever -shared -nopw -quiet -rfbport 5900 > /tmp/x11vnc.log 2>&1 &
websockify --web /usr/share/novnc 6080 localhost:5900 > /tmp/novnc.log 2>&1 &

echo "-----------------------------------------------------------"
echo " Bildschirm im Browser: http://localhost:6080/vnc.html"
echo " Bauen & starten:       cmake -B build && cmake --build build && ./build/hull"
echo "-----------------------------------------------------------"

exec "$@"
