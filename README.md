# RTSP Server

A GStreamer-based RTSP (Real Time Streaming Protocol) server that streams audio and video files over the network.

## Overview

This project implements an RTSP server that accepts local media files as input, processes them through GStreamer pipelines, and streams them to clients via RTSP. The server:

- Decodes multimedia files (supports any format handled by `decodebin`)
- Encodes video to VP8 format (640x480 resolution)
- Encodes audio to Opus format
- Serves the streams via RTSP protocol
- Allows multiple clients to connect to the same stream

## Features

- **Video Processing**: Automatic video scaling to 640x480 resolution with VP8 encoding
- **Audio Processing**: Audio conversion and resampling with Opus encoding
- **Network Streaming**: RTSP protocol support on port 8554
- **Stream Sharing**: Multiple clients can connect to the same media stream
- **File-based Input**: Accepts any media file supported by GStreamer's `decodebin`

## Prerequisites

- GStreamer 1.0 development libraries
- GStreamer RTSP Server 1.0 development libraries
- C++17 compiler
- Meson build system

### Installation on Linux

```bash
# Ubuntu/Debian
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly

# For RTSP Server
sudo apt-get install libgstrtspserver-1.0-dev
```

## Build Steps

### 1. Source the GStreamer environment (if using a custom installation)

```bash
source gst.env
```

### 2. Setup the build directory

```bash
meson setup build
```

### 3. Compile the project

```bash
meson compile -C build
```

The executable will be created at `./build/rtsp_server`.

## Usage

Run the server with a media file:

```bash
./build/rtsp_server <filename>
```

### Example

```bash
./build/rtsp_server test_video/Audio_Video_Sync_23,98_HEVC_2160p-by_PhotoJoseph.mov
```

The server will output:
```
RTSP server running at rtsp://127.0.0.1:8554/test
Streaming file: test_video/Audio_Video_Sync_23,98_HEVC_2160p-by_PhotoJoseph.mov
```

### Connecting to the Stream

Use any RTSP client to connect to the stream:

**VLC Media Player**
```
Open Network Stream → rtsp://127.0.0.1:8554/test
```

**ffplay**
```bash
ffplay rtsp://127.0.0.1:8554/test
```

**GStreamer**
```bash
gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:8554/test ! decodebin ! videoconvert ! autovideosink
```

## Project Structure

```
rtsp_server/
├── rtsp_server.cpp      # Main server implementation
├── meson.build          # Build configuration
├── gst.env              # GStreamer environment variables (for custom installations)
├── test_video/          # Sample video file for testing
└── README.md            # This file
```

## Pipeline Details

The server uses the following GStreamer pipeline:

```
filesrc location=<filename> !
decodebin name=decoder

decoder. ! 
queue ! 
videoconvert ! 
videoscale ! 
video/x-raw,width=640,height=480 !
vp8enc deadline=1 ! 
rtpvp8pay name=pay0 pt=96

decoder. ! 
queue ! 
audioconvert ! 
audioresample ! 
opusenc ! 
rtpopuspay name=pay1 pt=97
```

This pipeline:
- Reads a media file with `filesrc`
- Decodes both audio and video streams with `decodebin`
- For video: scales to 640x480, encodes with VP8
- For audio: converts and resamples, encodes with Opus
- Packages streams as RTP payloads for RTSP delivery

## Configuration

The RTSP server is configured with:
- **Port**: 8554 (default RTSP port)
- **Mount point**: `/test`
- **Video resolution**: 640x480
- **Video codec**: VP8
- **Audio codec**: Opus
- **VP8 encoding deadline**: 1 (low-latency mode)

## Troubleshooting

### Connection refused
Ensure the server is running and listening on port 8554. Check if another application is using the port:
```bash
lsof -i :8554
```

### GStreamer plugin not found
Ensure all required GStreamer plugins are installed or the `gst.env` environment is sourced.

### File not found error
Provide the full path to your media file or ensure it exists in the current working directory.

## License

This project is provided as-is for educational and streaming purposes.
