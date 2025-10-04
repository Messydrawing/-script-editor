# Visual Novel Editor Scaffold

This directory contains a Qt6-based scaffold for a modular visual novel script editor written in modern C++20. The project is organised into GUI, model, and export modules that are linked together by the root CMake project.

## Building

```bash
mkdir build
cd build
cmake -S .. -B .
cmake --build .
```

Qt6 with the Widgets component is required. The resulting executable is `visual_novel_editor`.
