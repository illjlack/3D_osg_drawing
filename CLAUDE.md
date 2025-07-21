# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

### Windows (Visual Studio)
```bash
# Quick build using vcpkg
build.cmd

# Manual CMake build
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux/macOS
```bash
# Quick build with automatic dependency detection
./build.sh

# Build options
./build.sh -d          # Debug build
./build.sh -c -d       # Clean and build debug
./build.sh -j 8        # Use 8 parallel jobs
```

### Development Setup
- Uses vcpkg for dependency management - ensure VCPKG_ROOT environment variable is set
- Requires Qt5 (5.15.16+), OpenSceneGraph (3.6.5+), and GLM
- C++17 standard required
- Visual Studio 2019+ on Windows, GCC 8+/Clang 10+ on Linux/macOS

## Architecture Overview

### Core Components
This is a Qt5-based 3D drawing application using OpenSceneGraph (OSG) for 3D rendering.

**Main Application Flow:**
- `src/main.cpp` - Application entry point with global configuration loading
- `src/ui/MainWindow` - Central UI coordinator with docking panels
- `src/ui/OSGWidget` - OpenSceneGraph rendering viewport
- `src/core/Common3D.h` - Global 3D settings and parameter management

**Geometry System:**
- `GeometryBase` - Abstract base class for all 3D objects using Qt's signal/slot system
- Concrete geometry classes in `src/core/geometry/` (Point3D, Line3D, Sphere3D, etc.)
- Factory pattern via `createGeo3D(DrawMode3D mode)` for object creation
- Manager-based architecture with specialized managers for different aspects:
  - `GeoStateManager` - Object state and lifecycle
  - `GeoNodeManager` - OSG scene graph integration  
  - `GeoRenderManager` - Rendering and visual representation
  - `GeoControlPointManager` - Interactive control points for editing

**Key Architectural Patterns:**
- **Manager Pattern**: Each Geo3D object has 4 specialized managers accessed via `mm_*()` methods
- **Parameters System**: Centralized `GeoParameters3D` struct controls all visual properties
- **Global Configuration**: `GlobalParametersManager` singleton for app-wide settings
- **Qt Signal/Slot**: Heavy use of Qt's event system for UI-geometry communication

### Picking System
The application has both legacy and modern picking implementations:
- `src/core/picking/OSGIndexPickingSystem` - Current index-based picking
- `src/core/picking_old/` - Legacy CPU-based picking system (kept for reference)

### Building System
Supports multiple architectural styles:
- Basic geometric primitives (points, lines, polygons, solids)
- Complex building types in `src/core/buildings/` (houses, domes, spires)
- `BuildingFactory` for procedural building generation

## Key Conventions

### File Organization
- Headers use `#pragma execution_character_set("utf-8")` for Chinese text support
- UI components in `src/ui/` with corresponding .h/.cpp pairs
- Core geometry in `src/core/geometry/` following naming pattern `[Type]3D.h/.cpp`
- Utility functions in `src/util/` for OSG integration, math, and I/O operations

### Manager Access Pattern
```cpp
// Access managers through Geo3D objects using mm_ prefix
auto geo = createGeo3D(DrawMode_Point3D);
geo->mm_state()->setState(GeoState_Drawing);
geo->mm_controlPoint()->addControlPoint(worldPos);
geo->mm_render()->updateGeometries();
```

### Global Settings Access
```cpp
// Access global parameter manager singleton
GlobalParametersManager& config = GlobalParametersManager::getInstance();
config.loadGlobalSettings("config/global_settings.cfg");
```

### Logging System
Uses custom LOG_* macros defined in LogManager:
```cpp
LOG_INFO("Message", "Category");
LOG_WARNING("Warning message", "Category");
LOG_ERROR("Error message", "Category");
```

## Testing and Quality

### Build Verification
Always run both build scripts to verify cross-platform compatibility:
- Test `build.cmd` on Windows
- Test `build.sh` on Linux/macOS if available
- Check that vcpkg dependencies resolve correctly

### Code Style
- Uses Chinese comments extensively (UTF-8 encoding)
- Qt naming conventions (camelCase for methods, PascalCase for classes)
- OSG reference counting via `osg::ref_ptr<>` for OSG objects
- STL containers for application data structures

## Common Development Tasks

### Adding New Geometry Types
1. Create `[Type]3D.h/.cpp` in `src/core/geometry/`
2. Inherit from `GeometryBase` and implement required virtual methods
3. Add corresponding `DrawMode3D` enum value in `Enums3D.h`
4. Update `createGeo3D()` factory function
5. Add to CMakeLists.txt geometry source lists

### Modifying UI
- Main interface layout in `MainWindow::setupUI()`
- Property editing in `PropertyEditor3D` (dockable panel)
- Tool selection in `ToolPanel3D` (dockable panel)
- Status updates through `StatusBar3D`

### Working with OSG Scene Graph
- All geometry rendering goes through OSG scene graph
- Use `osg::ref_ptr<>` for memory management
- Scene updates triggered through manager system
- Coordinate transformations handled in `OSGUtils`

### Configuration Management
- Global settings auto-save/load in main.cpp
- Per-object parameters through `GeoParameters3D`
- Preset system available through `GlobalParametersManager`