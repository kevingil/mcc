# OpenCraft parity

OpenCraft is a C and raylib game that follows Java Edition's behavior. It does not link the official game, and it does not speak the Mojang protocol. The art in `client/resources/` is the Good Vibes pack by Acaitart, CC-BY. Leave that credit in place. Do not add Mojang textures, sounds, or decompiled source.

Behavior target: Java Edition 26.3 (15 September 2026, protocol 777, data version 5023).

ID lists: `docs/parity/registry/`. They were generated on 10 October 2026 from the public misode/mcmeta summary dump, which had already moved to 26.4 Snapshot 3. Counts are in `registry/SUMMARY.md`. A published 26.3 list had 1,286 block ids, 1,658 item ids, and 161 entity types. This dump has 1,288, 1,662, and 163. Treat snapshot-only behavior as out of scope until it ships in a stable release.

## Where to look

| Doc | What it decides |
| --- | --- |
| `registry/SUMMARY.md` | Counts and how far the current client is |
| `registry/blocks.md`, `items.md`, `entities.md`, `biomes.md`, `catalogs.md` | One line per id |
| `SYSTEMS.md` | Gameplay systems that are not a single id |
| `PHYSICS.md` | Hitbox, tick, movement, jump, fluids |
| `WORLD.md` | Dimensions, height, light, border, clock |
| `SHADERS.md` | Raster shaders, and why ray tracing is last |
| `SERVER.md` | Headless server, protocol, VPS hosting |
| `SLICES.md` | The ordered work agents implement |
| `AGENT.md` | How an agent session takes one slice |

## What the client does today

The playable loop is a singleplayer world.

- Menus, world list, create, rename, delete. Multiplayer and Realms draw a coming-soon screen in `client/screen_multiplayer.c`.
- Chunks are 16 by 128 by 16. `CHUNK_HEIGHT` is 256 and is not the stored height. `WORLD_HEIGHT` in `client/voxel_types.h` is 128. Y starts at 0. Water level is 62.
- Terrain is one noise field, a shore spawn, trees, and sand under water. `client/world_generation.c`.
- 143 block ids exist as full cubes. `water` flows seven steps and can form a new source. That is the only `partial` block. 1,144 block ids are missing.
- The hotbar has nine slots. The inventory is a 9 by 5 creative list filled with those cubes. `bucket` and `water_bucket` are stored as block types.
- The player is 0.6 by 1.8 with the camera at 1.62, which matches the standing hitbox and eye height. Movement is per frame: walk 5.0 blocks/s, sprint 8.0, gravity 20, jump impulse 8. `client/player.c`. Java Edition's numbers are in `PHYSICS.md`.
- Saves: gzip NBT `level.dat`, edited chunks in `region/r.<x>.<z>.mca`. Unedited chunks are regenerated from the seed.
- Rendering is raylib's default material. `LoadMaterialDefault()` in `client/voxel_renderer.c`. No game shader, no ray tracing.
- There is no server binary.

Nothing in the registries is `done`. A cube is not parity.

## Rules that slices do not get to reopen

- One shared simulation. Singleplayer and the dedicated server run the same tick, block table, and fluid code. The client owns input, audio, and drawing.
- Protocol is OpenCraft's, version 1, in `SERVER.md`. Official clients are out of scope.
- String ids in the registries are the names in new data. Do not invent a second name for `grass_block`.
- Ray tracing is slice S30. It stays blocked until the raster game and the server slices it depends on are done.
- An agent session takes one slice, on one branch, in one PR. `AGENT.md`.
