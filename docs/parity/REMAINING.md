# Remaining Java Edition parity

This is the remaining-work list for the behavior target in `docs/parity/README.md`: Java Edition 26.3, the OpenCraft protocol, Good Vibes art, and no Mojang assets. It is a gap list. It is not a claim that `kevin/cursor/ui-parity-biomes-b258` is finished.

`docs/parity/README.md` ("What the client does today") is behind the client. These are already in the tree:

- `client/hud.c` draws the hotbar, the offhand slot, the crosshair, ten hearts, ten food icons, air bubbles, and the experience-bar sprite.
- `client/inventory_ui.c` opens a survival screen (2x2 craft grid, four armor icons, 27 storage slots, the hotbar, and an offhand slot) and a creative picker with category tabs, search, and a destroy slot.
- `client/crafting.c` matches 15 shaped and shapeless recipes, including logs to planks, sandstone, the crafting table, the chest, and the furnace. Using `BLOCK_CRAFTING_TABLE` opens the 3x3 grid.

The slice queue in `docs/parity/SLICES.md` is still `TODO` except S25 (`PARTIAL`) and S30 (`BLOCKED`). `docs/parity/registry/SUMMARY.md` still has 0 `done` ids: 1,288 blocks (1 partial, 143 cube, 1,144 missing), 1,662 items, 163 entity types, 3 dimensions. A cube is not parity.

## Landed on this branch, and still not parity

These are in the tree. None of them closes the slice it touches.

- F3 lines use the game font, `0xE0E0E0` on a `0x90505050` band. The frame graph shows only while Alt is held. The debug screen still lies about block light, the day cycle, and sounds.
- `RecolorBlockTile` in `client/voxel_renderer.c` shifts grass, dirt, sand, stone, leaves, planks, snow, ice, and water toward vanilla means at load time. The PNGs are unchanged. S32 is still open: there is no biome tint and no colormap.
- Non-tour `BiomeAt` uses temperature, humidity, continentalness, erosion, and weirdness. Nether, End, and cave biomes stay off the overworld surface. Height follows continentalness and erosion, rivers carve to the waterline, and a snow line follows temperature. The tour is still one strip per biome.
- `MCC_BIOME_TOUR` stamps one house per strip and walks the camera through the door. That is a tour prop, not a structure generator.

## Simulation

S01 and S02 are open. `UpdatePlayerPhysics` in `client/player.c` steps on `GetFrameTime()` with walk 5.0, sprint 8.0, gravity 20, and jump impulse 8. `PHYSICS.md` locks a 20 Hz tick, walk 4.317 blocks/s, sprint 5.612, sneak about 1.295, gravity 0.08, and a jump apex of 1.2522. Sneak, swim, and sleep hitboxes are absent. The standing box is already 0.6 by 1.8 with the camera at 1.62.

`client/hud.c` runs its own 20 Hz clock for the attack indicator and air supply. That clock does not drive movement, fluids, or the world.

S03 is open. Solid versus air, plus a water check, is what collision uses. There is no hardness, blast resistance, light emission, or friction table.

## Dimensions and height

S08 is open. `WORLD_HEIGHT` in `client/voxel_types.h` is 128. Chunks store `blocks[16][128][16]`, y from 0. `CHUNK_HEIGHT` is 256 and is not the array height. `WORLD.md` gives the overworld `min_y` -64 and height 384 (top block y 319), in 16-block sections. The Nether and the End are height 256. Saves in `client/world_save.c` require `WORLD_HEIGHT == SAVE_CHUNK_HEIGHT`. The F3 screen prints `height` from `WORLD_HEIGHT` and always labels the dimension `minecraft:overworld`.

## Lighting

S07 is open. `InitVoxelRenderer` in `client/voxel_renderer.c` bakes no light. `SkyLightAt` in `client/screen_gameplay.c` returns 15 when the column above the player is air or water, and 0 when a solid block is overhead. The F3 line is `Client Light: %d sky, 0 block`. There is no block-light channel, no nibble array, and no per-face level. `WORLD.md` wants sky light and block light, each 0 through 15, with a face at the max of the two.

## Day cycle

S11 is open. `DrawGameplayScreen` clears to a fixed sky blue, `(Color){135, 206, 235, 255}`. The F3 line is `Local Difficulty: 0.00 / 0.00 (no day cycle)`. There is no overworld clock. `WORLD.md` advances one tick per tick and wraps at 24,000. Nether and End clocks stay fixed. Weather is listed in `SYSTEMS.md` as missing and has no slice.

## Biomes

`client/biomes.c` has one surface palette per registry biome id (68). Outside the tour, `BiomeAt` picks an overworld biome from the climate fields, and `GetTerrainHeight` follows continentalness, erosion, and a river mask. S25 is still `PARTIAL`. Missing after this pass: caves, ore placement, and the 52 structure ids in `registry/SUMMARY.md`. `registry/SUMMARY.md` still says 68 biomes missing; that count is stale against `client/biomes.c`. Nether and End ids are still overworld palettes, not dimensions (S26, S27).

## Mobs and entities

S23 (pig) and S24 (zombie) are open. `registry/SUMMARY.md` lists 163 entity types, all missing: 92 mobs, 29 vehicles, 21 projectiles, 21 technical. No mob is simulated. `SYSTEMS.md` also leaves breeding, projectiles, vehicles, villagers, enchanting (43 ids), effects (41 ids), and experience missing. The experience bar in `client/hud.c` is the sprite only.

## Item entities

S05 is open. `HandleBlockBreaking` in `client/player.c` sets the targeted cube to air on click. The comment above the survival starter kit says broken blocks do not drop anything. There is no item-entity type. Dropping a held stack is deferred; see below.

## Recipe book

Deferred; see below. The grid matcher in `client/crafting.c` is separate from the book. S18 is still `TODO`: it wants `common/craft.c` shared with the server, and the three recipes oak log to planks, planks to sticks, and planks to a crafting table. Sticks are not in the current recipe table. S18 leaves the other recipe ids (2,055 in that slice's out-of-scope line) for later slices.

S06 is still `TODO` for the empty-inventory contract. `client/player.c` fills a new survival inventory from `starterKit` (planks, logs, stone, glass, a crafting table, a furnace, chests, and other cubes). The screen itself is the one in `client/inventory_ui.c`.

## Armor

The survival and creative inventory screens draw empty helmet, chestplate, leggings, and boots icons. `SlotMayPlace` in `client/inventory_ui.c` accepts craft, storage, hotbar, and offhand stacks. The comment on that function says armor slots only take armor, and there is no armor yet. S06 lists armor slots as out of scope. S24 lists armor as out of scope. No later slice owns armor items or the defense they would apply.

## Hunger drain

S20 is open. `DrawStatusBars` in `client/hud.c` always draws ten full hearts and ten full food icons. `Player` in `client/voxel_types.h` has `airSupply` and no food level or health field. Air does tick down while the eyes are in water, and the bubbles follow that value. Sprint is always allowed. `PHYSICS.md` blocks sprint at food level 6 once hunger exists. There is no starvation, no eating, and no `bread` item.

## Redstone

No slice owns redstone. `BLOCK_REDSTONE_ORE` and `BLOCK_REDSTONE_BLOCK` are full cubes. The creative "Redstone Blocks" tab lists `BLOCK_REDSTONE_BLOCK`. There is no dust, repeater, comparator, piston, or signal. `PHYSICS.md` says redstone schedules on the same 20 Hz clock as fluids, and that clock is S01.

## Block states and non-cube blocks

S09 is open. `registry/blocks.md` counts 1,144 missing ids and 143 cubes. One mesh shape is a full cube per id (`SYSTEMS.md`). `oak_stairs` and `oak_door` are missing, not `partial`. Cactus, chest, furnace, and the crafting table place as cubes. Water is the only `partial` block: `BLOCK_WATER` plus seven flow levels and a falling level in `client/voxel_types.h`, with heights from `WaterVisualHeight`. Collision for every solid id is the 1x1x1 box.

S04 is open. Hotbar and storage counts exist, and `bucket` / `water_bucket` are still `BlockType` values (`BLOCK_BUCKET`, `BLOCK_WATER_BUCKET`), not item ids with max count 1.

## Fluids beyond water

S10 is open. `client/water.c` spreads water seven steps and can form a source where two sources meet. Updates are a queue capped by `WATER_UPDATES_PER_TICK` (10), not one block per 5 ticks. Lava is absent. `PHYSICS.md` wants lava every 30 ticks in the overworld and every 10 in the Nether, and water that evaporates in the Nether. Swimming is the placeholder in `UpdatePlayerPhysics`: vertical velocity 4.5 while space is held, horizontal velocity multiplied by 0.55 per frame. `registry/SUMMARY.md` lists 5 fluids, water partial, lava missing.

## Nether and End as dimensions

S26 and S27 are open. The client has one overworld. Nether and End biome ids in `client/biomes.c` (`nether_wastes`, `crimson_forest`, `warped_forest`, `soul_sand_valley`, `basalt_deltas`, `the_end`, `end_highlands`, `end_midlands`, `end_barrens`, `small_end_islands`) are surface palettes in that same height range. There is no portal, no coordinate scale of 8, and no fixed Nether or End clock. `WORLD.md` is the dimension table those slices implement.

## Multiplayer protocol

A session path is already described in `SLICES.md` ("Session slice") and `SERVER.md` ("Session packets"). The server listens, accounts live in `opencraft.db`, and joined clients share a seed plus block edits. Inventory packet id 7 carries the hotbar and 45 storage stacks. Block ids on the wire are `u16` `BlockType` values.

The parity slices on top of that path are still open:

| Id | Gap still in the docs |
| --- | --- |
| S12 | Headless world simulation. `SYSTEMS.md`: the server counts a 20 Hz loop and does not simulate the voxel world. `client/voxel_types.h` still includes raylib types. |
| S13 | Protocol v1 section chunks and Input. Chunk packets are not sent. Clients generate terrain locally. |
| S14 | Direct connect that sends Input and draws server chunks. The session screen is the account path, not that Input loop. |
| S15 | Server list refresh via Hello. `SYSTEMS.md`: `servers.txt` exists; Realms stays a coming-soon screen. |
| S16 | Server-owned break timer. `SYSTEMS.md`: the placing client also draws the edit immediately. |
| S17 | A second player body. Other players are missing. |
| S31 | `common/db` completion API and Kore `GET /health` on 8080. SQLite still steps on the server thread. |

v1 in `SERVER.md` has no chat, encryption, or compression. Official Mojang clients stay out of scope.

## Sounds

No slice owns audio. `client/raylib_game.c` loads `resources/coin.wav` and the menus and the gameplay screen play it on clicks. The F3 line is hardcoded `SC: 0  Sounds: 0/0 (Mood 0%)`. `SYSTEMS.md` says music is not loaded, and a sound slice can follow the tick (S01). It is not on the critical path.

## Shaders

S28, S29, and S30. `client/voxel_renderer.c` uses `LoadMaterialDefault()` for the opaque mesh and the transparent mesh. `SHADERS.md` records no GLSL files under `client/resources/shaders/`. S28 is the chunk shader (atlas sample times baked light). S29 is the scrolling water shader. S30 (software ray tracing, off by default) is `BLOCKED` on S17, S28, and S29.

## Not in this branch

Two items were explicitly deferred.

**Recipe book.** `Player.recipeBookOpen` in `client/voxel_types.h` is set to 0 in `InitPlayer` and never read. `CraftingRecipe.group` in `client/crafting.h` is the field a shared book entry would use. No book button, no book page, and no recipe list beyond the 15 grid matches.

**Dropping item stacks.** Breaking a block does not spawn a stack, and the player has no drop key. `QuickMoveOnce` in `client/inventory_ui.c` will not take a craft result unless the whole stack fits in the hotbar and storage, because there is nowhere to drop the rest. S05 is the slice that later puts dirt, stone, and oak-log drops into the inventory. An item entity on the ground is still absent.
