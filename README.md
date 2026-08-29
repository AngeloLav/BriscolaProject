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

Run the application by providing the folder of a game. Every supported video
inside that folder is processed in filename order; one video corresponds to
one round. The ground-truth CSV can be inside the game folder or next to it with the same name as the folder:

```bash
./build/briscola <game_folder>
```

Example:

```bash
./build/briscola data/game1
```

Expected layout:

```text
data/
├── game1/
│   ├── round01.mp4
│   ├── round02.mp4
│   └── prediction.json       # optional temporary test input
└── game1.csv
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
