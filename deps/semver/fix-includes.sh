#!/bin/bash

if $(grep -q '<semver.h>' semver.c); then
  sed 's/semver.h/semver\/semver.h/g' -i semver.c
fi
