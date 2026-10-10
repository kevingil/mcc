# Slices

Work top to bottom. Status is `TODO`, `DOING`, `BLOCKED`, or `DONE`. An agent session takes the first `TODO` whose blockers are `DONE`, and no other slice. The rules are in `AGENT.md`.

Contracts: `PHYSICS.md`, `WORLD.md`, `SERVER.md`, `SHADERS.md`.

## Queue

| Id | Slice | Status | Blocked by |
| --- | --- | --- | --- |
| S01 | Fixed tick | TODO | |
| S02 | Player movement | TODO | S01 |
| S03 | Block properties | TODO | S02 |
| S04 | Item stacks | TODO | S03 |
| S05 | Breaking dirt, stone, and oak logs | TODO | S02, S04 |
| S06 | Survival inventory | TODO | S04 |
| S07 | Light | TODO | S03 |
| S08 | World height | TODO | S03 |
| S09 | Stairs and door state | TODO | S08 |
| S10 | Lava | TODO | S01, S09 |
| S11 | Daylight | TODO | S07 |
| S12 | Headless server | TODO | S02, S04 |
| S13 | Protocol v1 | TODO | S08, S12 |
| S14 | Direct connect | TODO | S13 |
| S15 | Server list | TODO | S14 |
| S16 | Authoritative edits | TODO | S05, S14 |
| S17 | Second player | TODO | S16 |
| S18 | Crafting | TODO | S06, S17 |
| S19 | Pickaxe | TODO | S05, S17 |
| S20 | Hunger | TODO | S06, S17 |
| S21 | Furnace | TODO | S18 |
| S22 | Chest | TODO | S06, S17 |
| S23 | Pig | TODO | S17 |
| S24 | Zombie | TODO | S07, S19, S23 |
| S25 | Four biomes | TODO | S08, S17 |
| S26 | Nether | TODO | S10, S25 |
| S27 | End | TODO | S26 |
| S28 | Chunk shader | TODO | S07, S11 |
| S29 | Water shader | TODO | S10, S28 |
| S30 | Ray tracing | BLOCKED | S17, S28, S29 |

Take the earliest slice in that table whose status is `TODO` and whose blockers are all `DONE`. One session does not take a second slice. S12 is the wide header split. It stays one session and one PR.

## S01: Fixed tick

Status: TODO
Blocked by: none
Owns: `client/screen_gameplay.c`, `game/tick.c`, `game/tick.h`, `tests/tick_test.c`, `CMakeLists.txt`

The gameplay screen accumulates `GetFrameTime()` and calls the simulation at 20 Hz. Rendering still happens once per frame. A stall runs at most five catch-up ticks, then drops the rest.

Done when:

- [ ] `tests/tick_test` steps 20 ticks and reports one second of simulation time.
- [ ] The gameplay screen uses that stepper. `UpdatePlayer` is not on a raw frame delta for movement after S02, so this slice only has to introduce the clock and call the existing update from it.
- [ ] `bash .agents/build.sh` succeeds.

Out of scope: changing walk speed, fluids, or the mesh.

## S02: Player movement

Status: TODO
Blocked by: S01
Owns: `game/physics.c`, `game/physics.h`, `client/player.c`, `tests/movement_test.c`, `CMakeLists.txt`
Contract: `PHYSICS.md`

Replace the frame-time gravity in `client/player.c` with the tick recurrence. Standing hitbox stays 0.6 by 1.8. Eye stays 1.62.

Done when `tests/movement_test` passes all of these on flat stone, with no window:

- [ ] Five seconds of forward input covers 4.317 * 5 blocks, within 1%.
- [ ] Sprint covers 5.612 blocks/s, within 1%.
- [ ] Sneak covers 1.295 blocks/s, within 1%.
- [ ] A jump reaches 1.2522 blocks, within 0.01.
- [ ] The player does not step up a one-block wall.
- [ ] `bash .agents/test.sh` still shows a world. Movement in that world uses the tick.

Out of scope: hunger, sprint cancel, water current, sneaking the hitbox down to 1.5. Those have later owners. Air friction 0.91 is in scope. Ice friction is not.

## S03: Block properties

Status: TODO
Blocked by: S02
Owns: `game/block_props.c`, `game/block_props.h`, `tests/block_props_test.c`, `CMakeLists.txt`

A table keyed by the string ids we already implement, plus air. Each row has solid, transparent, collision (full cube or none), hardness, blast resistance, light emission, friction, and drop id. `IsBlockSolid` and `IsBlockTransparent` read the table. Unknown ids are air.

Done when:

- [ ] `tests/block_props_test` checks stone, dirt, oak log, glass, water, and air against the table.
- [ ] Water is not solid and not a light source. Glowstone emits 15. Glass is not opaque.
- [ ] The 143 cube ids still place.

Out of scope: mining time (S05), stairs (S09), generating ores.

## S04: Item stacks

Status: TODO
Blocked by: S03
Owns: `game/item.c`, `game/item.h`, `client/player.c`, `tests/item_test.c`, `CMakeLists.txt`

An item stack is a string id, a count, and a max count. Most ids stack to 64. `bucket` and `water_bucket` stack to 1 and are no longer `BlockType` values. The hotbar stores stacks. Place and pick-up use the stack. The creative grid may still list the cube blocks until S06.

Done when:

- [ ] `tests/item_test` fills a stack, rejects a 65th dirt, and swaps a bucket for a water bucket when a source is picked up.
- [ ] The hotbar bucket still picks up and places a source in game.

Out of scope: the survival layout, tools, food.

## S05: Breaking dirt, stone, and oak logs

Status: TODO
Blocked by: S02, S04
Owns: `game/break.c`, `game/break.h`, `client/player.c`, `tests/break_test.c`, `CMakeLists.txt`

Hold to break. Time comes from hardness in the S03 table and an empty hand. Dirt, stone, and oak log drop their item into the player inventory. Other cubes keep today's instant break until a later tool slice says otherwise. Reach is 4.5.

Done when:

- [ ] `tests/break_test` shows stone taking longer than dirt, and an oak log dropping `oak_log`.
- [ ] In game, a broken stone cube is in the inventory and the world cell is air.

Out of scope: pickaxes (S19), animation, particles.

## S06: Survival inventory

Status: TODO
Blocked by: S04
Owns: `client/player.c`, `client/screen_gameplay.c`

Replace the creative fill with 36 storage slots plus the 9 hotbar slots. New worlds start empty except a water bucket in the hotbar, so the current water test still has a bucket. E still opens the screen. The mouse still moves stacks.

Done when:

- [ ] A new world does not contain a stack of every cube.
- [ ] Shift-click moves a stack between the hotbar and the storage rows.
- [ ] `bash .agents/test.sh` passes.

Out of scope: crafting grid (S18), armor slots.

## S07: Light

Status: TODO
Blocked by: S03
Owns: `game/light.c`, `game/light.h`, `client/voxel_renderer.c`, `tests/light_test.c`, `CMakeLists.txt`
Contract: `WORLD.md`

Sky light and block light, 0 through 15, stored per section cell once sections exist. Until S08, store them on the current 128-high chunk and rebuild in S08. Bake the max of the two channels into the mesh. Sunlight at noon on an open face is 15. Glowstone is 15. A solid cube blocks sky light.

Done when:

- [ ] `tests/light_test` covers a skylit column, a covered column, and a glowstone in a sealed room.
- [ ] A sealed room without glowstone renders darker than the surface.

Out of scope: the chunk shader (S28), mob spawning.

## S08: World height

Status: TODO
Blocked by: S03
Owns: `client/voxel_types.h`, `client/voxel_world.c`, `client/world_generation.c`, `client/world_save.c`, `client/chunk_codec.c`, `client/chunk_codec.h`, `tests/save_format_test.c`
Contract: `WORLD.md`

Store the overworld as 16-block sections from y=-64 through y=319. Old region files load into the y range they were saved with. New worlds generate a surface near the old water level, expressed in the new coordinates (sea level 63 in modern terms; keep the current shore behavior, shifted into the new space). `CHUNK_HEIGHT` either matches the stored height or goes away.

Done when:

- [ ] `tests/save_format_test` round-trips a section below y=0 and a section above y=127.
- [ ] An existing save from before this slice still loads the chunks it had edited.
- [ ] `bash .agents/test.sh` passes.

Out of scope: the Nether, biomes, a new terrain generator.

## S09: Stairs and door state

Status: TODO
Blocked by: S08
Owns: `game/block_state.c`, `game/block_state.h`, `client/voxel_renderer.c`, `client/player.c`, `tests/block_state_test.c`, `CMakeLists.txt`

`oak_stairs` and `oak_door` store facing and the door's open bit. Collision is not a full cube. The mesher draws those two ids from their state. Placement faces the player. Every later stair copies this, rather than a new mesher path.

Done when:

- [ ] `tests/block_state_test` places a stair and a door and reads the facing back.
- [ ] The player can walk up a stair and cannot walk through a closed door.
- [ ] `oak_stairs` and `oak_door` in `registry/blocks.md` are `partial`.

Out of scope: the other wood types, waterlogged state.

## S10: Lava

Status: TODO
Blocked by: S01, S09
Owns: `client/water.c`, `client/water.h`, `game/fluid.c`, `tests/fluid_test.c`, `CMakeLists.txt`
Contract: `PHYSICS.md`, `WORLD.md`

Move water onto the 5-tick schedule without changing the seven-step rule or the two-source rule. Add lava: 30 ticks in the overworld, 10 ticks in the nether flag, shorter reach than water. Lava burns a player who stands in it, once health exists. Until S20, contact sets a flag the later slice reads.

Done when:

- [ ] `tests/fluid_test` shows water advancing one block per 5 ticks and lava per 30.
- [ ] A lava source in game flows and is not swimmable.
- [ ] `registry/blocks.md` marks `lava` partial.

Out of scope: buckets of lava (follow-up after this slice if the bucket path is still water-only), Nether dimension (S26).

## S11: Daylight

Status: TODO
Blocked by: S07
Owns: `game/clock.c`, `client/voxel_renderer.c`, `client/screen_gameplay.c`, `tests/clock_test.c`, `CMakeLists.txt`
Contract: `WORLD.md`

The overworld clock advances with the tick. Sky and fog move between the overworld colors. Night reduces baked sky light. The Nether and End flags exist on the clock and do not advance.

Done when:

- [ ] `tests/clock_test` wraps at 24,000 ticks.
- [ ] Standing still for a few in-game hours changes the clear color. The smoke test still sees a lit frame at noon.

Out of scope: weather, a custom shader.

## S12: Headless server

Status: TODO
Blocked by: S02, S04
Owns: `client/voxel_types.h`, `game/` headers that still include raylib, `server/main.c`, `CMakeLists.txt`, `client/player.c` only for the include split

`opencraft-server` loads a world directory, runs 20 ticks, flushes, and exits. It does not link raylib and does not open a socket. Shared headers stop including `raylib.h`. The client converts at the boundary.

Done when:

- [ ] The server binary completes 20 ticks with no display.
- [ ] `bash .agents/test.sh` passes. Singleplayer still places and breaks a block.

Out of scope: packets, protocol, hosting files.

## S13: Protocol v1

Status: TODO
Blocked by: S08, S12
Owns: `net/protocol.c`, `net/protocol.h`, `server/main.c`, `tests/protocol_test.c`, `CMakeLists.txt`
Contract: `SERVER.md`

Implement the frame layout and packet ids from `SERVER.md`. The server accepts one client, sends Welcome and the chunks around the spawn, and applies Input to the player.

Done when:

- [ ] `tests/protocol_test` frames a Hello and a Chunk and rejects protocol 2.
- [ ] A tiny client in the test connects, sends Input, and reads a moved position back. No raylib.

Out of scope: the real client UI (S14), a second player, encryption.

## S14: Direct connect

Status: TODO
Blocked by: S13
Owns: `client/screen_multiplayer.c`, `client/screen_gameplay.c`, `net/client.c`, `net/client.h`

The direct-connect field takes host, port, and name. The gameplay screen sends Input and draws server chunks. Leave closes the socket. Singleplayer still loads local saves. Realms stays coming soon. Join Server stays disabled.

Done when:

- [ ] Two processes, server and client, on localhost: the client walks and the server log shows the tick.
- [ ] Cancel and a bad port return to the title without crashing.
- [ ] `bash .agents/test.sh` passes for singleplayer.

Out of scope: the server list, authoritative breaking (the client may still predict a block until S16; do not add a new local-only block type).

## S15: Server list

Status: TODO
Blocked by: S14
Owns: `client/screen_multiplayer.c`, `net/server_list.c`, `net/server_list.h`

Enable Add Server, Edit, and Join Server. The list is a local file of name, host, and port. Refresh opens a short Hello and shows a failure if the socket dies. Direct Connect stays.

Done when:

- [ ] A saved entry survives a restart of the client.
- [ ] Join uses the same path as direct connect.
- [ ] `bash .agents/test.sh` passes.

Out of scope: ping MOTD art, Realms.

## S16: Authoritative edits

Status: TODO
Blocked by: S05, S14
Owns: `server/main.c`, `net/protocol.c`, `client/screen_gameplay.c`, `tests/protocol_test.c`

Attack and use send buttons. The server runs the break timer and placement. The client applies Block packets and does not write the world itself while connected.

Done when:

- [ ] `tests/protocol_test` places a block and reads the Block packet.
- [ ] Two clients see the same new block. A client that ignores the packet and draws locally is a test failure, so the test disconnects the writer and checks the server world.

Out of scope: inventory sync beyond the hotbar stack the server already tracks. A full inventory sync belongs here only if placement needs it. Do not design a second inventory protocol.

## S17: Second player

Status: TODO
Blocked by: S16
Owns: `server/main.c`, `net/protocol.c`, `client/voxel_renderer.c`, `tests/protocol_test.c`

Each connection is an entity. Entity packets move the other player. Both clients see both names.

Done when:

- [ ] `tests/protocol_test` connects two Hells and sees two Entity moves.
- [ ] On localhost, walking on one client moves a body on the other.

Out of scope: skins, chat, collision between players. They may pass through each other in this slice.

## S18: Crafting

Status: TODO
Blocked by: S06, S17
Owns: `game/craft.c`, `game/craft.h`, `client/screen_gameplay.c`, `tests/craft_test.c`, `CMakeLists.txt`

2x2 in the inventory and 3x3 on `crafting_table`. Recipes: oak log to oak planks (4), planks to sticks (4), planks to a crafting table. The table UI opens when the player uses that block. Server and client share `craft.c`.

Done when:

- [ ] `tests/craft_test` covers those three recipes and rejects a shapeless guess.
- [ ] In singleplayer, a log becomes planks. On a server, the same recipe changes the server inventory.

Out of scope: the rest of the recipe book (2,055 recipe ids). Add recipes in later slices that need them.

## S19: Pickaxe

Status: TODO
Blocked by: S05, S17
Owns: `game/break.c`, `game/item.c`, `tests/break_test.c`, `game/craft.c`

Wooden pickaxe recipe from planks and sticks. Stone breaks faster with it than by hand and drops cobblestone. The empty hand no longer instant-breaks stone. Dirt still breaks by hand.

Done when:

- [ ] `tests/break_test` covers hand versus wooden pickaxe on stone.
- [ ] The crafted pickaxe shows up in the hotbar and survives a save.

Out of scope: the other tiers and the other tools.

## S20: Hunger

Status: TODO
Blocked by: S06, S17
Owns: `game/hunger.c`, `game/hunger.h`, `client/player.c`, `client/screen_gameplay.c`, `tests/hunger_test.c`, `CMakeLists.txt`

Food level starts at 20. Sprint is refused at 6 or below. Eating a cooked piece is out of scope until a food item exists. This slice adds `bread` as a craft from wheat only if wheat exists. If wheat does not exist, add a debug grant of one bread in the test and a single `bread` item that restores hunger. Do not add farming.

Done when:

- [ ] `tests/hunger_test` drains hunger while sprinting and stops the sprint at 6.
- [ ] The bar is visible in game.

Out of scope: starvation damage, saturation details beyond "eating bread moves the bar."

## S21: Furnace

Status: TODO
Blocked by: S18
Owns: `game/furnace.c`, `game/furnace.h`, `client/screen_gameplay.c`, `tests/furnace_test.c`, `CMakeLists.txt`

`furnace` opens a three-slot UI. One recipe: raw iron is not in the game yet, so smelt `sand` into nothing? No. Smelt `oak_log` into `charcoal` and consume a plank as fuel. Lock that in the test. The block entity is stored on the chunk.

Done when:

- [ ] `tests/furnace_test` finishes one charcoal given fuel and time.
- [ ] The result is in the output slot in game, and it reloads from the save.

Out of scope: the rest of the smelting list.

## S22: Chest

Status: TODO
Blocked by: S06, S17
Owns: `game/chest.c`, `game/chest.h`, `client/screen_gameplay.c`, `tests/chest_test.c`, `CMakeLists.txt`

`chest` opens 27 slots. Contents save with the chunk. Two players on a server see the same contents. A double chest is out of scope.

Done when:

- [ ] `tests/chest_test` writes a stack and reads it back from the chunk save.
- [ ] In game, the items are still there after leaving and rejoining.

Out of scope: trapped chests, hoppers.

## S23: Pig

Status: TODO
Blocked by: S17
Owns: `game/entity.c`, `game/entity.h`, `game/pig.c`, `client/voxel_renderer.c`, `tests/pig_test.c`, `CMakeLists.txt`

One passive mob. It wanders, it has the pig hitbox from the Hitbox page (the test records the width and height it uses), and it replicates through the Entity packet with a kind field extended in this slice. Document the kind values in `SERVER.md` in this same PR. A player punch with an empty hand applies knockback. Drops wait for S24's damage helper if the pig dies. This slice can leave the pig unkilled.

Done when:

- [ ] `tests/pig_test` spawns a pig and sees it move over 100 ticks.
- [ ] A second client sees the pig move.
- [ ] `registry/entities.md` marks `pig` partial.

Out of scope: breeding, saddles, other mobs.

## S24: Zombie

Status: TODO
Blocked by: S07, S19, S23
Owns: `game/zombie.c`, `game/health.c`, `tests/zombie_test.c`, `CMakeLists.txt`

Zombies spawn on the surface when sky light is low, walk toward a player within follow range, and deal damage. The player has 20 health. Dying respawns at the spawn point and drops nothing in this slice.

Done when:

- [ ] `tests/zombie_test` does not spawn a zombie at sky light 15, does spawn one at sky light 0, and reduces health on contact.
- [ ] `zombie` is partial in the registry.

Out of scope: armor, baby zombies, reinforcements, other hostiles.

## S25: Four biomes

Status: TODO
Blocked by: S08, S17
Owns: `client/world_generation.c`, `client/world_generation.h`, `tests/biome_test.c`, `CMakeLists.txt`

Columns are plains, forest, desert, or ocean. Forest places oak trees. Desert places sand and cactus. Ocean is water to sea level. Plains are grass. The same seed is stable across a reload. Other biome ids stay missing.

Done when:

- [ ] `tests/biome_test` forces each of the four and checks the surface block.
- [ ] Those four lines in `registry/biomes.md` are partial.

Out of scope: rivers, temperature, the other 64 biomes, structures.

## S26: Nether

Status: TODO
Blocked by: S10, S25
Owns: `game/dimension.c`, `client/world_generation.c`, `server/main.c`, `client/screen_gameplay.c`, `tests/nether_test.c`

A nether dimension with the height and flags in `WORLD.md`. A lit obsidian portal moves the player, dividing x and z by 8. Water placed there evaporates. Lava uses the fast period. Return travel multiplies by 8.

Done when:

- [ ] `tests/nether_test` converts coordinates and evaporates a water source.
- [ ] In game, a portal transfers the player and a second portal brings them back.

Out of scope: nether fortresses, mobs, the full biome set.

## S27: End

Status: TODO
Blocked by: S26
Owns: `game/dimension.c`, `client/world_generation.c`, `tests/end_test.c`

An end dimension with a stone island at the origin and the fixed clock from `WORLD.md`. No dragon, no end cities, no outer islands. The player gets there by a debug item or a test hook this slice documents in `SYSTEMS.md`. Do not pretend a complete end portal frame exists unless this slice actually builds one. Prefer the test hook and a single end-portal block that teleports, marked partial.

Done when:

- [ ] `tests/end_test` loads the dimension, finds end stone at the origin, and does not advance the clock.
- [ ] The player can return to the overworld spawn.

Out of scope: the dragon fight, shulkers, elytra.

## S28: Chunk shader

Status: TODO
Blocked by: S07, S11
Owns: `client/resources/shaders/chunk.vs`, `client/resources/shaders/chunk.fs`, `client/voxel_renderer.c`
Contract: `SHADERS.md`

GLSL 330. The shader samples the atlas and the baked light. Noon on the surface matches the overworld sky. A glowstone in a sealed room is brighter than the surrounding stone.

Done when:

- [ ] The shader loads, and a missing shader file falls back to the default material so the game still runs.
- [ ] `bash .agents/test.sh` passes.
- [ ] A sealed dark room and a lit surface are distinguishable. Record that in the PR with a screenshot.

Out of scope: ray tracing, shadows, post processing.

## S29: Water shader

Status: TODO
Blocked by: S10, S28
Owns: `client/resources/shaders/water.fs`, `client/voxel_renderer.c`

Flowing water uses the height the simulation already computed and a scrolling UV so the flow direction is visible.

Done when:

- [ ] A placed source still spreads, and the surface moves.
- [ ] `bash .agents/test.sh` passes.

Out of scope: reflections, caustics, lava shading beyond a tint.

## S30: Ray tracing

Status: BLOCKED
Blocked by: S17, S28, S29
Owns: `client/trace/trace.c`, `client/trace/trace.h`, `client/voxel_renderer.c`, `tests/trace_test.c`, `CMakeLists.txt`
Contract: `SHADERS.md`

Do not start this slice while its status is `BLOCKED`. A maintainer sets it to `TODO` only after S17, S28, and S29 are `DONE`.

Software DDA from the camera through section storage. Off by default. A debug key toggles it. The center ray matches the raster target block. Booting the game shows the raster world. No hardware ray-tracing extension.

Done when:

- [ ] `tests/trace_test` hits a known block along a known ray.
- [ ] The debug view toggles back to the raster mesh without reloading the world.
- [ ] `bash .agents/test.sh` passes with the tracer left off.

Out of scope: path tracing, reflections, replacing the raster renderer.
