#!/bin/bash

meson setup build --prefix ${PWD}/build
meson compile -C build

