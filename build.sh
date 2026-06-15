#!/usr/bin/env bash
set -eu

mkdir -p build

RAYLIB_DIR="third-party/raylib/src"
OUT="build/screenshot"

make -C "$RAYLIB_DIR" PLATFORM=PLATFORM_DESKTOP

case "$(uname -s)" in
Linux)
  clang \
    main.c \
    "$RAYLIB_DIR/libraylib.a" \
    -I"$RAYLIB_DIR" \
    -lGL \
    -lm \
    -lpthread \
    -ldl \
    -lrt \
    -lX11 \
    -o "$OUT"
  ;;

Darwin)
  clang \
    main.c \
    capture_macos.m \
    "$RAYLIB_DIR/libraylib.a" \
    -I"$RAYLIB_DIR" \
    -framework OpenGL \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreAudio \
    -framework CoreVideo \
    -framework Foundation \
    -framework ScreenCaptureKit \
    -framework CoreGraphics \
    -o "$OUT"
  ;;

MINGW* | MSYS* | CYGWIN*)
  OUT="build/screenshot.exe"

  clang \
    main.c \
    "$RAYLIB_DIR/libraylib.a" \
    -I"$RAYLIB_DIR" \
    -lopengl32 \
    -lgdi32 \
    -lwinmm \
    -o "$OUT"
  ;;

*)
  echo "Unsupported OS: $(uname -s)"
  exit 1
  ;;
esac

"$OUT"
