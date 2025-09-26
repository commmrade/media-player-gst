# GStreamer Media Player

A command-line media player built with GStreamer for learning purposes. This player supports both audio and video files with various filters and effects.

## Features

- **Audio & Video Playback**: Supports common media formats (MP4, MKV, MOV, AVI, FLV, WebM, WMV, MPEG, M2TS, PNG, JPEG, JPG)
- **Audio Effects**:
  - Volume control (0.0 - 1.0)
  - Audio balance/panorama (left-right channel balance)
  - Low-pass and high-pass filters with configurable cutoff frequency
  - Echo/reverb effects with adjustable delay, feedback, and intensity
  - Pitch adjustment
  - Playback speed control
  - Noise reduction
- **Video Effects**:
  - Video balance (saturation control)
  - Color inversion
  - Grayscale conversion
- **Flexible Input**: Supports local files and remote URLs (HTTP/HTTPS)
- **Smart Detection**: Automatically detects media type based on file extension

## Dependencies

- GStreamer 1.0 development libraries
- CMake 3.10.0 or higher
- GLib development libraries
- pkg-config

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                 libgstreamer-plugins-good1.0-dev libgstreamer-plugins-bad1.0-dev \
                 cmake build-essential pkg-config
```

**Fedora/CentOS/RHEL:**
```bash
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel \
                 gstreamer1-plugins-good-devel gstreamer1-plugins-bad-devel \
                 cmake gcc pkg-config
```

**Arch Linux:**
```bash
sudo pacman -S gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad \
               cmake gcc pkg-config
```

## Building

1. Clone the repository:
```bash
git clone <repository-url>
cd media-player-gst
```

2. Create a build directory and compile:
```bash
mkdir build
cd build
cmake ..
make
```

The executable will be created as `proj` in the build directory.

## Usage

### Basic Syntax
```bash
./proj --path <media-file> [options]
```

### Command Line Options

| Option | Argument | Description |
|--------|----------|-------------|
| `--path` | `<file>` | Path to media file (local or URL) |
| `--audio` | - | Force audio-only playback |
| `--video` | - | Force video playback |
| `--volume` | `<0.0-1.0>` | Set audio volume |
| `--balance` | `<-1.0-1.0>` | Audio balance (left/right) |
| `--lowpass` | - | Enable low-pass filter |
| `--highpass` | - | Enable high-pass filter |
| `--cutoff` | `<frequency>` | Cutoff frequency for filters (Hz) |
| `--delay` | `<nanoseconds>` | Echo delay |
| `--feedback` | `<0.0-1.0>` | Echo feedback amount |
| `--intensity` | `<0.0-1.0>` | Echo intensity |
| `--speed` | `<rate>` | Playback speed (1.0 = normal) |
| `--pitch` | `<pitch>` | Audio pitch adjustment |
| `--grayscale` | `<value>` | Convert to grayscale |
| `--colorinvert` | `<value>` | Invert colors |
| `--noisethreshold` | `<0.0-1.0>` | Noise reduction threshold |

### Examples

**Play a video file:**
```bash
./proj --path /path/to/video.mp4
```

**Play audio with volume control:**
```bash
./proj --path /path/to/audio.mp3 --volume 0.5
```

**Play with echo effect:**
```bash
./proj --path /path/to/audio.mp3 --delay 500000000 --feedback 0.3 --intensity 0.7
```

**Play at double speed:**
```bash
./proj --path /path/to/video.mp4 --speed 2.0
```

**Play remote stream:**
```bash
./proj --path https://example.com/stream.mp4
```

**Audio with low-pass filter:**
```bash
./proj --path /path/to/audio.mp3 --lowpass --cutoff 1000
```

**Video with color effects:**
```bash
./proj --path /path/to/video.mp4 --colorinvert 1 --grayscale 0.5
```

## Architecture

The player is structured into several modules:

- **main.c**: Main application logic and GStreamer pipeline management
- **settings.h/settings.c**: Command-line argument parsing and configuration
- **state.h/state.c**: Pipeline state management and element linking
- **CMakeLists.txt**: Build configuration

The application uses a GStreamer pipeline with dynamic pad linking to handle various media formats automatically.

## Supported Formats

**Video**: MP4, MKV, MOV, AVI, FLV, WebM, WMV, MPEG, MPG, M2TS
**Audio**: MP3, WAV, FLAC, OGG, AAC, and other formats supported by GStreamer
**Images**: PNG, JPEG, JPG

Format detection is automatic based on file extension.

## License

This project is licensed under the GNU General Public License v3.0. See the [LICENSE](LICENSE) file for details.

## Troubleshooting

**Missing GStreamer plugins**: Install additional plugin packages if you encounter unsupported format errors:
```bash
# Ubuntu/Debian
sudo apt install gstreamer1.0-plugins-ugly gstreamer1.0-libav

# Fedora
sudo dnf install gstreamer1-plugins-ugly gstreamer1-libav
```

**Build errors**: Ensure all development packages are installed and pkg-config can find GStreamer:
```bash
pkg-config --modversion gstreamer-1.0
```
```
