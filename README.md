# Asm Profiler (chxrcky.asmprof)

Geode mod for Geometry Dash 2.2091 on Geode 6.0.0-alpha.1.

- `src/asm/ticks_arm64.S` - AArch64 assembly: cycle-accurate tick read, counter frequency, moving average
- `src/ticks.hpp` - C declarations for the asm, plus a C++ fallback for other targets
- `src/main.cpp` - hooks `GJBaseGameLayer::update` and shows time spent per update in the HUD
- `CMakeLists.txt` - enables ASM, builds the .S file only on Android arm64

Put your `logo.png` in the project root. Build the same way as your other mods (Geode CLI 4.0.0+).
