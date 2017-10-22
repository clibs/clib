#!/bin/sh

color_tests() {
  cmd="./clib-search $1"
  stdout=$($cmd)
  # lame check for color
  case "$stdout" in
    *"[39;49;90;49"*)
      echo >&2 "Expected \`${cmd}\` to suppress all color"
      exit 1
      ;;
  esac
}

color_tests --no-color
color_tests -n
