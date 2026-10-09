# Source Bridge Notes

Planned integration points for a full Source SDK plugin:

1. Call `PortalCraft_Initialize()` during plugin load.
2. Call `PortalCraft_OnFrame()` once per client or render frame.
3. When the Portal Gun trace resolves, call `PortalCraft_OnPortalGunHit(...)`.
4. Replace the placeholder `writeSourceViewProxy(...)` logic with actual camera/player transform application.

Current files:

- `include/bridge_shared.hpp`: exact shared memory layout shared with Java.
- `src/shared_memory_win.*`: Windows named file mapping wrapper.
- `src/portalcraft_plugin.cpp`: exported bridge functions and event emission.
