# BriscolaProject

C++ application for video-based detection and analysis of Briscola playing cards.

## Requirements

* CMake
* OpenCV

Additional dependencies required for model inference will be documented as they are introduced.

## Build

Clone the repository and configure the project:

```bash
git clone https://github.com/AngeloLav/BriscolaProject.git

cmake -S . -B build
cmake --build build
```

After modifying the source code, rebuilding only requires:

```bash
cmake --build build
```

## Usage

Run the application by providing a video file:

```bash
./build/briscola <video_path>
```

Example:

```bash
./build/briscola videos/test.mp4
```

Press `ESC` to stop execution.

## Development workflow

Before starting work:

```bash
git pull
```

Commit and publish changes:

```bash
git add .
git commit -m "Description of the changes"
git push
```

__Build files, local videos and other generated resources are excluded through `.gitignore`.__
