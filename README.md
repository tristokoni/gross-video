# Gross Video — macOS OpenFX prototype

Personal rhythmic video retiming effect intended for DaVinci Resolve Free. No audio processing. **Resolve compatibility has not yet been tested.** Requires the host to supply frames at arbitrary times via OpenFX temporal clip access. Tested here: C++17 Linux compilation and timing unit tests, not macOS loading or rendering in Resolve.

## Build and install on your Mac

Requires macOS 11+ and Apple's Command Line Tools (`xcode-select --install` if missing). Quit Resolve, open Terminal in this project directory, and run:

```sh
bash scripts/install-mac.sh
```

Builds a universal Apple Silicon / Intel bundle, signs it locally, and installs it in `/Library/OFX/Plugins`. The installation asks for your Mac password through sudo. On updates it moves the previous bundle into `/Library/OFX/GrossVideo-backups/` before installing. For build only: `bash scripts/build-mac.sh`. No paid developer account is needed for this local build.

Restart Resolve, find **Gross Video** in Effects, apply it to a video clip, and inspect its effect controls. If it is absent, check Resolve's Video Plugins preferences and plugin loading errors. Host support must be verified with your exact Resolve Free version.

## Controls

- Preset: Normal, Stutter x4, Reverse, Freeze, Ping pong, Custom steps.
- BPM: manually match the music. There is no audio beat detection.
- Beats: cycle length in quarter-note beats; default 4.
- Origin frame: effect time at the first cycle boundary. Adjust this to align to the clip/music; host time is not necessarily zero at the clip start.
- Step 01–16: source positions in the current cycle, from 0 to 1. Select Custom steps to use them. Each step holds its selected source frame for 1/16 of the cycle. For example 0.5 selects halfway through the cycle. This is a step sequencer, not a graphical curve editor.

Freeze holds the first frame of each cycle; Stutter repeats the first quarter four times. Reverse and ping-pong operate within each cycle. Source times are clamped to the clip's available range. Audio remains untouched. Resolve parameter animation can be used where supported. Save customized settings using the host's preset facilities if available; plugin file preset import/export is not implemented.

## First Resolve validation

Use a short clip showing frame numbers, at 24 fps, BPM 120, Beats 4, with Origin aligned to its first frame. A cycle is 48 frames. Check Normal against the original; Reverse should request 47 down to 0; Freeze should hold frame 0; Stutter should repeat 0–11 four times. Check scrubbing in random order and export a short section. Test Custom steps set entirely to 0.5 (frame 24 for the first cycle). If Resolve fails to provide temporal frames, record the Resolve version and its error: a successful build alone does not establish host compatibility.

## Development

```sh
mkdir -p build
c++ -std=c++17 -Wall -Wextra -Werror tests/timing.cpp -o build/timing-tests
./build/timing-tests
```

CPU renderer, float RGBA only; no frame interpolation, GPU acceleration or graphical curve editor in this prototype. OpenFX headers are vendored under their upstream license in `vendor/openfx/LICENSE.md`; upstream revision is recorded in `vendor/openfx/REVISION`. No changes to the reusable cloud configuration are required by this project.

## Additional presets and updating

Chaos and Complex are original video patterns inspired by rhythmic time manipulation, not copies of Gross Beat's proprietary presets. Existing preset indices are preserved for saved Resolve projects.

| Preset | Motion |
| --- | --- |
| Chaos | 16 shuffled slices with selected slices reversed |
| Complex | Repeats, forward slices, holds and a reversed ending |
| Stutter x8 / x16 | Repeat the first eighth / sixteenth of the cycle |
| Reverse chops | Reverse each sixteenth independently |
| Half speed / Double speed | Half-speed motion / two passes per cycle |
| Tape stop | Gradually slow to a stop within each cycle |
| Speed up | Accelerate through the cycle |
| Bounce chops | Forward and backward within each quarter |
| Shuffle | Reorder 16 moving slices |
| Glitch hold | Rhythmic held frames and jumps |

Patterns repeat every cycle and give the same result during scrubbing and export. At low frame rates or very fast BPM, short slices can contain only one or two frames.

Quit Resolve, then update from your existing project folder:

```sh
git pull origin main
bash scripts/install-mac.sh
```

Restart Resolve to load the expanded preset menu. The installer preserves the previous plugin outside the scanned Plugins directory.
