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

Run the application by providing the folder of a game. Video inside that folder is processed in filename order; one video corresponds to one round. The ground-truth CSV have to be inside the game folder:

```bash
./build/briscola <game_folder>
```

Example:

```bash
./build/briscola data/game1/
```

Expected layout:

```text
data/
 ── game1/
    ├── round01.mp4
    ├── round02.mp4
    └── game1.csv
```

