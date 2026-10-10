# Setup and smoke test

These scripts are the shared way to install, build, and check MC.C. Use them from any editor or automation. Tool-specific directories (`.cursor`, `.codex`, `.claude`, and the others listed in `.gitignore`) stay untracked.

Run everything from a checkout of this repo. The scripts find the repo root themselves.

## Setup

```sh
bash .agents/setup.sh
```

Installs the Ubuntu packages the desktop build needs: a C/C++ toolchain, CMake, X11, OpenGL, ALSA, plus Xvfb, xdotool, and ImageMagick for the smoke test. Running it again is a no-op aside from apt's own update.

The Cloud Agent image points `c++` at clang, and that clang links against the GCC 14 libstdc++. `libstdc++-14-dev` is in the package list for that reason.

## Build

```sh
bash .agents/build.sh
```

Runs `cmake -S . -B build` and `cmake --build build`. The first configure downloads raylib 5.0. The client binary is `build/mcc/mcc`. The server binary is `build/server/opencraft-server`.

## Run

Resources load relative to the working directory. Start the game from `client/`:

```sh
cd client
../build/mcc/mcc
```

The server has no resources and no window:

```sh
./build/server/opencraft-server
```

Enter on the title screen starts the world. The rest of the controls are in the project README. With no sound card, raylib prints ALSA warnings and continues.

## Smoke test

```sh
bash .agents/test.sh
```

Builds, starts the game on its own Xvfb, waits through the logo, and checks the title screen is drawn. It then presses Enter, waits until block textures load and the first burst of chunk meshes finishes, and checks the world frame is not blank. The process has to still be running at the end.

Screenshots and logs go to `/tmp/mcc-test`. Set `MCC_TEST_OUT` to write them somewhere else.

```sh
MCC_TEST_OUT=/tmp/mcc-test bash .agents/test.sh
```

A missing window, an early exit, or a black frame fails the script.

## Parity work

`docs/parity/README.md` is the map. A session that implements gameplay reads `docs/parity/AGENT.md` and one slice in `docs/parity/SLICES.md`, then stops. Ray tracing is slice S30 and stays blocked.

## Cloud Agent install

The Cloud Agent install command is `bash .agents/setup.sh`. Nothing needs to stay running after boot. Build and run the smoke test when you need to check a change.
