#!/bin/bash

if [ "$1" = "uci" ]; then
  ./build/sockfish-uci
else
  ./build/sockfish-sdl
fi

