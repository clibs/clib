#!/bin/bash

color_tests() {
  local opt="$1"
  local cmd="./clib-search ${opt}"
  local stdout=`${cmd}`
  # lame check for color
  if [[ $stdout == *"[39;49;90;49"* ]]; then
    echo >&2 "Expected \`${cmd}\` to surpress all color";
    exit 1
  fi
}

color_tests --no-color
color_tests -n
