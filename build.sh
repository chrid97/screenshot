set -eu

mkdir -p build

make -C third-party/raylib/src PLATFORM=PLATFORM_DESKTOP

clang \
  main.c \
  third-party/raylib/src/libraylib.a \
  -Ithird-party/raylib/src \
  -lGL \
  -lm \
  -lpthread \
  -ldl \
  -lrt \
  -lX11 \
  -o build/screenshot

./build/screenshot
