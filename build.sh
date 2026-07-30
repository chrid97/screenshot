#!/usr/bin/env bash
set -eu

mkdir -p build

RAYLIB_DIR="third-party/raylib/src"
OUT="build/screenshot"

# ============================================================================
#  Shared flags
# ============================================================================
DEBUG_CFLAGS="-O0 -g3 -Wall -Wextra -Wshadow -Wconversion \
-fsanitize=address,undefined"

RELEASE_CFLAGS="-O3 -DNDEBUG -march=native \
-Wall -Wextra -Wshadow -Wconversion"

CFLAGS=$RELEASE_CFLAGS
# ============================================================================
# ##############
# ============================================================================

make -C "$RAYLIB_DIR" PLATFORM=PLATFORM_DESKTOP

case "$(uname -s)" in
Linux)
  clang $CFLAGS \
    main.c \
    capture_linux.c \
    "$RAYLIB_DIR/libraylib.a" \
    -I"$RAYLIB_DIR" \
    -Ithird-party \
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
    -Ithird-party \
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
    -Ithird-party \
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

# ============================================================================
#  TODO
# ============================================================================
# shared include for third party?
