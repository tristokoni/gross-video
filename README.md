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

## Independent video fade and custom controls (1.1)

**Timing** groups the original controls and a new Time enabled checkbox. Disable it to use only the fade section. Original presets and parameter identifiers are preserved. **Custom time** contains the original 16 source-position steps plus interpolation: Hold preserves the existing sequencer; Linear connects positions; Smooth eases between them. Interpolation wraps from the last point to the first. This interpolates source time, not pixels; frame selection remains discrete.

**Video fade** is disabled by default and can be combined with any time preset:

- Fade preset: Gate, Pulse, Fade in, Fade out, Triangle, Sidechain, Strobe, Double pulse, Tremolo, Custom envelope.
- Fade target: Black dims RGB while preserving alpha; Opacity fades transparency to reveal a lower video track. With no lower track, transparent areas may also appear black.
- Fade beats controls its own cycle length using shared BPM and Origin frame.
- Fade phase offsets that cycle (0–1); Fade depth blends the effect (0 bypasses); Fade minimum controls its lowest level.
- Fade duty controls gate/pulse width and sidechain recovery time. It does not change Fade in/out, Triangle, Tremolo or Custom envelope.
- Custom fade offers 16 brightness/opacity points (0–1) with independent Hold, Linear or Smooth interpolation. Choose Custom envelope to activate them.

No audio input or sidechain detection is required: Sidechain is a repeating recovery envelope synchronized to the manual BPM. Controls can be animated using Resolve where supported. This update uses grouped host controls, not a drawn curve editor.

Performance improvements: full-row copies instead of per-pixel copies, custom points read only when selected, one source-time calculation per frame request, and contiguous row processing. Temporal remapping always uses the render action, including when fade is disabled. Fade processes contiguous float RGBA rows and preserves HDR values and premultiplication. CPU only; actual speed depends on Resolve's decoder/cache and temporal frame requests. No macOS performance measurement has been made.

Additional validation:

```sh
c++ -std=c++17 -O2 -Wall -Wextra -Werror tests/envelope.cpp -o build/envelope-tests
./build/envelope-tests
```

In Resolve verify an existing project is unchanged with Fade disabled; then try Gate and Sidechain, disable Time to isolate the fade, and test Opacity over a second video track. Check scrubbing and export. New controls still require validation in Resolve on macOS.

### Time-only rendering fix

Removed temporal identity delegation: Resolve could skip time modulation when fade was disabled, zero depth, or minimum 1. All combinations now use the same source-frame request and rendering path. Full-row copy optimization remains active when gain is 1. Restart Resolve after installing; if a clip still shows an old result, clear its render cache. Validate Reverse and Stutter with Fade enabled off, then on at depth 0, then at nonzero depth.

## Render reliability update (1.2)

`Safe frame fallback` is on by default in Timing. When Resolve cannot supply a requested temporal frame, the plugin tries the current output-time source frame instead. This can avoid end-of-clip failures, but that individual frame will lose the intended time modulation. Disable it for strict rendering that fails rather than substituting a frame. Both potential requests are declared to the host in FramesNeeded. If the current frame is also unavailable, the error is propagated; decoder, Fusion graph and GPU failures are not hidden. Failed requests report output time, requested source time and status to stderr (availability in Resolve logs depends on how Resolve was launched).

The clip range is treated as inclusive, as specified by OFX. Fractional bounds are restricted to valid whole frames; invalid/nonfinite metadata is not used for clamping. No unconditional last-frame subtraction is applied. Null buffers, incompatible image formats and insufficient row stride are rejected before copying pixels.

Additional CPU improvements: reuse BPM/length/origin reads, bypass time math for Normal, skip envelope evaluation for neutral fade, poll cancellation every 32 scanlines, build macOS with `-O3`. Temporal source-frame decoding can still dominate runtime. This is not a GPU implementation, and no Resolve/macOS speedup has been measured.

The reported reproducible end-of-render Fusion composition error has not been reproduced in this Linux environment. Test the failing section after installation and clearing its old render cache. A fallback may change the picture slightly; inspect the last frames. If export still fails, retain the exact frame/timecode, Resolve version, active presets and full error text for diagnosis.

## Stability, scratch presets and music analysis (1.3)

Fixed an OFX API violation in parameter description: the plugin used `paramGetHandle` during DescribeInContext, where instance parameter handles are forbidden. A host rejecting those calls could leave a stale property handle, assigning group parents incorrectly (including self-parenting). Groups now exist before children and parenting uses only the descriptor property handles returned by `paramDefine`. A strict mock host test rejects descriptor-time instance access and validates the hierarchy. This is a concrete defect; the reported macOS save crash has not been reproduced or confirmed resolved here. Check saving/reopening a copy of your project before replacing your working project. If an old instance retains malformed UI state, test removing and re-adding the updated effect in that copy.

Numeric parameter read failures now stop the action rather than using uninitialized handles, and nonfinite parameter values are rejected. Render layout checks also reject empty or excessively large bounds before offset arithmetic. No persistent frame buffers or audio data are kept between actions. Hardware/codec limits still apply; this does not guarantee performance on every device.

Eight appended presets preserve existing preset indices: Vinyl scratch, Baby scratch, Transformer scratch, Scratch rewind, Glitch burst, Glitch staircase, Beat juggle, Broken tape. They manipulate video time only.

### Analyze music button

Standard OpenFX image effects do not receive timeline audio. Under **Music sync**, choose **Music WAV**, set **Music start frame** to the effect-time frame where that file starts, then press **Analyze music**. The button analyzes at most the first 30 seconds and writes BPM and Origin frame keyframes at the current playhead time. If timing controls already contain animation, these keyframes coexist with it; inspect or remove unwanted prior keyframes for a constant tempo.

Supported files: ordinary RIFF WAV, PCM 16/24-bit or IEEE float32, 8–192 kHz, up to 8 channels, at least 8 seconds. MP3, AAC and extensible/RF64 WAV are not supported; export a compatible WAV first. The file is only read when the button is clicked. Rendering and reopening the project use the saved numeric settings without reopening the audio file.

The lightweight detector uses onset-energy autocorrelation, roughly 60–182 BPM. It estimates steady tempo and an initial strong onset, not the musical downbeat, variable tempo, or the audio track position in Resolve. It may choose half/double tempo; check the result by ear and adjust BPM/Origin. Silence or ambiguous rhythm reports an error without changing controls. Analysis is synchronous and bounded; the UI may briefly pause on slower devices.

Validated on Linux: all timing/envelope/render support tests, strict OFX descriptor regression, identity regression, synthetic 120 BPM WAV analysis and rejection of silence/missing files, under AddressSanitizer and UndefinedBehaviorSanitizer. Actual macOS save/reopen, new preset playback and button integration require Resolve validation.

## Streamlined controls and render overhead (1.4)

Timing opens by default. Custom time points appear only when Time enabled is on and Custom steps is selected. Custom fade points appear only with Fade enabled and Custom envelope selected. Fade controls are disabled when fade is off; pulse width/recovery is hidden for presets that do not use it. This uses standard OFX Inspector visibility/enabled properties; exact appearance depends on Resolve. Existing saved parameter names and preset indices remain unchanged.

- **Align here** sets the beat anchor to the playhead frame.
- **Reset time pattern** sets the 16 custom positions to ascending steps.
- **Reset fade pattern** sets all 16 levels to fully visible.
- Buttons create keyframes at the current playhead and group their changes into one undo edit. Other keyframes remain intact.
- Clearer labels distinguish Time preset, Time cycle (beats), Beat anchor (frame), Fade amount and Fade cycle (beats). Tooltips explain normalized positions and fallback behavior. Frame slider display ranges are narrower while typed values retain the full permitted range.

Parameter reads now use a bounded stack cache for each OFX action, with no heap allocations for that cache. Duplicate host handle lookups and value samples at the same time are reused; values never persist across actions. Thread-local scope pointers isolate concurrent render threads and restore state after nested callbacks. Changing time or parameter set invalidates the cache. Horizontal image intersections are computed once per frame instead of once per row. Successful fallback frames no longer emit a log line each; failed source requests still report diagnostics.

A mock-host regression verifies 100 repeated reads produce one lookup and one sample, fresh reads at new times/instances, nested scopes, control visibility, button edits and undo brackets. This is a host-call reduction measurement, not a Resolve FPS benchmark. Source decoding, memory bandwidth and GPU/host processing can still dominate. UI/save/reopen integration and performance on macOS remain to be checked in Resolve. No custom drawn curve editor or GPU renderer is introduced.
