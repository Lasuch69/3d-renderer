# 3D Renderer

## Overview

Custom rendering engine powered by Vulkan API. I look forward into extending it into a small game engine.


## Platform Support

Currently only Linux is supported.


## Compiling

#### Tools

- Fedora: `sudo dnf install cmake g++`

#### Dependencies

- Fedora: `sudo dnf install vulkan-devel vulkan-validation-layers glfw-devel glm-devel`

#### Building

To build binary:

```
cmake . && cmake --build .
```
