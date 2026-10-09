# PortalCraft Prototype Scaffold

This workspace contains a starting scaffold for a `Portal + Minecraft` mashup:

- [`ARCHITECTURE.md`](file:///C:/Users/valer/OneDrive/Документы/Trae/temp_20261009_01/ARCHITECTURE.md) documents the split-process design.
- [`portalcraft-fabric`](file:///C:/Users/valer/OneDrive/Документы/Trae/temp_20261009_01/portalcraft-fabric) contains the Fabric 1.21.1 client-side bridge.
- [`portalcraft-source`](file:///C:/Users/valer/OneDrive/Документы/Trae/temp_20261009_01/portalcraft-source) contains the Windows C++ shared-memory bridge for Source-side integration.

## Notes

- The Fabric side currently publishes player transform every render frame through a Mixin on `MinecraftClient.render`.
- The Source side exposes exported functions that can be wired into a real Source SDK plugin lifecycle.
- Account-auth bypass, cracked-launcher support, and `online-mode = false` changes were intentionally not added.
