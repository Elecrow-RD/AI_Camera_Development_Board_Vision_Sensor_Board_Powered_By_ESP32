# Application modules

This directory contains code extracted from the original Arduino sketch.

- `ai`: device activation, AI chat, and image recognition
- `display`: LVGL callbacks and display-related helpers
- `iot`: device identity and AI IoT entity registration
- `hardware`: AI Camera V1.1 board-level pin mapping and controls
- `tasks`: FreeRTOS background task entry points
- `ui`: runtime UI state synchronization

Arduino only compiles subdirectories under the conventional `src` directory automatically. Because the existing `src` directory is intentionally left untouched, the main sketch includes these implementation files explicitly. A later refactoring step can replace the shared global state with explicit module interfaces.
