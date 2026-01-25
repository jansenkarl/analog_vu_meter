# AnalogVUMeterQt

A Linux desktop application that visually replicates a classic analog stereo VU meter (needle-style) using Qt 6 custom painting and PulseAudio capture.

## Features

- Stereo (Left/Right) analog VU meters with a retro hardware aesthetic
- RMS-based level measurement
- Classic VU ballistics
  - ~300 ms rise time
  - slower decay
  - slight peak overshoot
- Refresh rate ~60 Hz
- Audio capture runs outside the GUI thread
- **System output monitoring** (captures what you hear through speakers)
- Microphone input support
- Full PipeWire compatibility on Linux

## Dependencies

- C++20 compiler (GCC 11+ or Clang 14+ recommended)
- CMake 3.20+
- Qt 6 (Widgets)
- PulseAudio (libpulse)

On Debian/Ubuntu:

```bash
sudo apt-get install -y build-essential cmake pkg-config qt6-base-dev libpulse-dev
```

On Fedora:

```bash
sudo dnf install -y @development-tools cmake pkgconf-pkg-config qt6-qtbase-devel pulseaudio-libs-devel
```

On Arch Linux:

```bash
sudo pacman -S base-devel cmake pkgconf qt6-base pulseaudio
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```bash
./build/analog_vu_meter
```

### Audio Source Options

**System Output (Default)** - Monitors what you hear:
```bash
./build/analog_vu_meter
# or explicitly:
./build/analog_vu_meter --device-type 0
```

**Microphone Input**:
```bash
./build/analog_vu_meter --device-type 1
```

**Specific Device**:
```bash
./build/analog_vu_meter --device-name "alsa_output.pci-0000_00_1b.0.analog-stereo.monitor"
```

List available options:
```bash
./build/analog_vu_meter --list-devices
```

## Calibration / assumptions

- `0 VU` is calibrated to `-18 dBFS` by default for microphone input
- For system output monitoring, uses `-4.5 dBFS` reference for optimal meter range
- Meter scale is clamped to `[-20, +3]` VU for display
- The analog scale spacing is approximated with a piecewise non-linear mapping

You can change the reference with:

```bash
./build/analog_vu_meter --ref-dbfs -18
```

## Command Line Options

- `--list-devices` - Show available audio devices and usage
- `--device-type <0|1>` - 0=sink monitor (system output), 1=source (microphone)
- `--device-name <name>` - Specific PulseAudio device name
- `--ref-dbfs <db>` - Reference dBFS for 0 VU (default: -18)
