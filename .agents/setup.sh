#!/usr/bin/env bash
# Install the Ubuntu packages needed to build and smoke-test MC.C.
# Safe to run more than once.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

packages=(
  build-essential
  g++
  # The default c++ on the Cloud Agent image is clang, and it links against
  # the GCC 14 libstdc++ even when g++-13 is the compiler you invoke by hand.
  libstdc++-14-dev
  cmake
  pkg-config
  git
  ca-certificates
  libgl1-mesa-dev
  libglu1-mesa-dev
  libx11-dev
  libxrandr-dev
  libxi-dev
  libxinerama-dev
  libxcursor-dev
  libxkbcommon-dev
  libwayland-dev
  libasound2-dev
  xvfb
  xdotool
  x11-utils
  imagemagick
)

if [ "$(id -u)" -eq 0 ]; then
  apt-get update
  apt-get install -y --no-install-recommends "${packages[@]}"
else
  sudo apt-get update
  sudo apt-get install -y --no-install-recommends "${packages[@]}"
fi
