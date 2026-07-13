#!/bin/bash

TYPE=debug

if [ "$1" = "--release" ]; then
  TYPE=release
fi

meson setup build       \
  --prefix ${PWD}/build \
  --buildtype "$TYPE"   \

meson compile -C build

