# JBot (chxrcky.JBot) 

Geode mod for Geometry Dash 2.2091 on Geode 6.0.0-alpha.1

Shows a small cocos2d panel in levels (top-left) with the avg / peak / min time, which i think is useless tbh.
spent in `GJBaseGameLayer::update`. The panel itself is written in AArch64 assembly.

- `src/asm/panel_arm64.S` - the panel stats, building the cocos2d nodes (CCLayerColor + 4 CCLabelBMFont),
  layout, integer formatting and text updates. Calls cocos2d-x through mangled symbols.
- `src/asm/ticks_arm64.S` - cycle-accurate tick read, counter frequency, moving average
- `src/ticks.hpp` - C declarations for the asm, plus a C++ fallback (no panel) for other targets
- `src/main.cpp` - only the `$modify` hook that times `update` and calls `asmprof_panel_frame`
  (a Geode hook cannot be written in assembly)
- `CMakeLists.txt` - enables ASM, builds the .S files only on Android arm64
