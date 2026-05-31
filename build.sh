#!/bin/bash
set -e

if [ ! -d "build" ]; then
    mkdir "build"
fi

if [ ! -f "build/tiny_linux.bin" ] || [ "$1" == "-u" ] || [ "$2" == "-u" ]; then
    if [ -f "build/tiny_linux.bin" ]; then
        echo "Updating tiny builder..."
        rm build/tiny_linux.bin
    else
        echo "Downloading tiny builder..."
    fi
    cd build
    wget -q https://github.com/JHeflinger/tiny/raw/refs/heads/main/bin/tiny_linux.bin
    chmod +x tiny_linux.bin
    cd ..
fi

PROTO_DIR="protocols"
WD="$(pkg-config --variable=pkgdatadir wayland-protocols 2>/dev/null || echo /usr/share/wayland-protocols)"
WLR_XML="$PROTO_DIR/wlr-layer-shell-unstable-v1.xml"

echo "Generating Wayland protocol glue..."
wayland-scanner client-header "$WD/stable/xdg-shell/xdg-shell.xml" "$PROTO_DIR/xdg-shell-client-protocol.h"
wayland-scanner private-code "$WD/stable/xdg-shell/xdg-shell.xml" "$PROTO_DIR/xdg-shell-protocol.c"
wayland-scanner client-header "$WD/unstable/xdg-output/xdg-output-unstable-v1.xml" "$PROTO_DIR/xdg-output-unstable-v1-client-protocol.h"
wayland-scanner private-code "$WD/unstable/xdg-output/xdg-output-unstable-v1.xml" "$PROTO_DIR/xdg-output-unstable-v1-protocol.c"
wayland-scanner client-header "$WLR_XML" "$PROTO_DIR/wlr-layer-shell-unstable-v1-client-protocol.h"
wayland-scanner private-code "$WLR_XML" "$PROTO_DIR/wlr-layer-shell-unstable-v1-protocol.c"

PROD=""
if [ "$1" == "-p" ] || [ "$2" == "-p" ]; then
    PROD="-prod"
fi
./build/tiny_linux.bin -a $PROD
