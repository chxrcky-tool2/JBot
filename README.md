# JBot (chxrcky.JBot)

Geode mod for Geometry Dash 2.2091 on Geode 6.0.0-alpha.1

Open the pause menu and press the JBot button in the top right to get the panel.

Panel features:
- Record, Play, Stop for the replay engine, plus Save and Load by macro name (`.jb` files in the mod save folder, `macros/`)
- Noclip
- Speedhack, typed in, 0.01x to 4x, with optional speedhack audio (FMOD master pitch)
- TPS bypass, typed in, with an on/off toggle

Layout:
- `src/asm/replay_arm64.S` - the replay engine in AArch64 assembly: input buffer, record, truncate, seek, poll, load
- `src/asm/panel_arm64.S` - the in-level update-time panel
- `src/asm/ticks_arm64.S` - tick counter and moving average
- `src/replay.hpp` - C declarations for the replay engine
- `src/replay_fallback.cpp` - C++ version of the same engine, only built when the assembly is not
- `src/hooks.cpp` - noclip, speedhack, speedhack audio, TPS bypass and the replay hooks
- `src/panel.cpp` - pause menu button and the panel popup
- `src/macro_io.cpp` - `.jb` save and load
- `src/state.hpp` - shared settings
- `CMakeLists.txt` - builds the `.S` files only on Android arm64
- augh I'm tired 'src/state.hpp' hello

`.jb` format: 16 byte header (`JBM1`, float tps, u32 input count, u32 reserved), then 8 bytes per input (u32 frame, u32 flags: bit 0 down, bit 1 player 2, bits 8-15 button).
