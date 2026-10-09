# PortalCraft Mashup Architecture

## Purpose

This prototype splits gameplay into two cooperating processes:

- `Minecraft Fabric 1.21.1` is the authoritative host for player physics, collision, block interaction, inventory, and portal teleport rules.
- `Portal / Source SDK` is the visual guest that renders Aperture spaces, first-person weapon view, UI framing, and Portal-specific effects.

The bridge is inspired by a parallel-process architecture: both executables run at the same time and exchange state through a shared memory region on Windows.

## High-Level Topology

1. Minecraft writes current player transform and velocity into a named shared memory block every render frame.
2. Source reads that state and applies it to its local camera/player proxy.
3. Source writes Portal Gun impact events into a lock-free ring buffer inside the same memory block.
4. Minecraft consumes these events on the client thread, creates or updates portal anchors, and performs teleportation while preserving momentum.

## Process Roles

### Minecraft Host

- Owns world simulation, collision, inventory, and survival interactions.
- Builds invisible collision proxy geometry for Portal chambers.
- Resolves portal entry, exit transform, and momentum carry-over.
- Publishes:
  - position `x/y/z`
  - look vector / yaw / pitch
  - velocity
  - frame counter

### Source Guest

- Renders Aperture walls, effects, Portal Gun, and HUD shell.
- Receives host transform every frame.
- Detects Portal Gun firing and trace hit results.
- Publishes:
  - blue/orange portal hit events
  - hit position
  - surface normal
  - event sequence id

## Shared Memory Layout

Single named Windows mapping: `Local\\PortalCraftBridge`

Sections:

1. `BridgeHeader`
2. `MinecraftToSource PlayerState`
3. `SourceToMinecraft PlayerState`
4. `PortalEvent[64]` ring buffer

All fields are little-endian and fixed-size so both Java and C++ can map the same memory without serialization.

## Room-To-Block Synchronization

Portal BSP geometry should be voxelized into a collision mask used by Minecraft:

1. Parse BSP brushes / displacement surfaces for the active test chamber.
2. Convert world geometry into a voxel occupancy grid using a configurable scale, for example `1 Source unit -> 1/16 block`.
3. Mark voxels intersecting structural Portal surfaces as `BEDROCK` or another unbreakable technical block.
4. Fill everything else with normal Minecraft terrain or leave it editable.
5. During chunk generation or room import, bake this mask into a dedicated collision layer.

Recommended rule set:

- Walls, floors, ceilings, catwalk shells, observation glass frames: unbreakable.
- Decorative props and non-structural content: optional or ignored in the first prototype.
- Non-Aperture terrain outside chamber bounds: normal Minecraft blocks.

## Portal Mechanics Mapping

1. Source traces the Portal Gun shot.
2. On hit, Source writes a `PortalEvent`.
3. Minecraft reads the event and validates the target surface.
4. Minecraft creates an invisible rectangular trigger plane and stores:
   - center point
   - forward normal
   - local basis vectors
   - linked portal id
5. When the player's bounding box crosses the plane:
   - transform position into exit portal local space
   - rotate velocity from entry basis to exit basis
   - preserve speed magnitude
   - add minimal positional offset to avoid immediate re-trigger

## First Prototype Scope

Included in this scaffold:

- Fabric client mod structure
- Mixin that publishes player position/look each render frame
- Java shared memory access layer
- C++ Windows shared memory wrapper
- Source plugin skeleton for frame sync and portal event emission

Not included yet:

- Full Source SDK hook integration for camera overwrite
- BSP parser / voxelizer implementation
- Final portal collision plane system in Minecraft
- Any account-auth bypass or piracy-related setup

## Security / Legality Note

This scaffold is limited to legitimate local development and prototyping. It does not disable account validation, alter authentication mode, or add support for cracked distributions.
