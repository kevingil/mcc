# Systems

Registries list ids. This file lists the systems an id plugs into. Status is for the system, not for every id inside it. When a slice finishes a system, update the row here and the ids it actually implemented.

`missing` means the client has no behavior. `partial` means the note in the row. `done` is not used yet.

## Simulation

| System | Status | Now | First slice |
| --- | --- | --- | --- |
| Fixed 20 Hz tick, partial tick for drawing, stored subtick | missing | Frame-time integration in `client/player.c` | S01 |
| Player movement and jump | partial | Box and eye height match. Speeds do not | S02 |
| Block properties | missing | Solid versus air, plus a water check | S03 |
| Item stacks | partial | Cubes and two bucket types share `BlockType` | S04 |
| Breaking and drops | missing | A click deletes the cube | S05 |
| Inventory | partial | Survival screen uses the container texture. Hotbar uses the widget selector. 2x2 crafting sits in the inventory. | S06 |
| Light | missing | Meshes are fully lit | S07 |
| Height and sections | missing | y 0..127 stored | S08 |
| Block states | missing | One mesh shape per id | S09 |
| Fluids | partial | Water spreads seven steps. No schedule, no lava | S10 |
| Daylight | missing | No clock | S11 |
| Weather | missing | | later |
| Block entities | missing | Chest and furnace are cubes | S21, S22 |

## Creatures and items

| System | Status | First slice |
| --- | --- | --- |
| Crafting | partial | Inventory 2x2 and the crafting table 3x3. Recipe list selects logs, planks, sandstone, furnace, and chest. Not the full recipe book. |
| Tools and combat stats | missing | S19 |
| Hunger and food | missing | S20 |
| Smelting | missing | S21 |
| Mob AI | missing | S23 pig, S24 zombie |
| Breeding, taming, riding | missing | after S23 |
| Projectiles | missing | after S24 |
| Vehicles | missing | after the server can replicate an entity |
| Villagers and trades | missing | |
| Enchanting and anvils | missing | 43 enchantment ids in the catalog |
| Potions and effects | missing | 41 effect ids |
| Experience | missing | |

92 mob ids are listed in `registry/entities.md`. Textures under `client/resources/textures/entity/` do not count as progress.

## World

| System | Status | First slice |
| --- | --- | --- |
| Biomes | partial | Every registry id has a surface palette. `MCC_BIOME_TOUR` walks them. Normal seeds also pick a palette. Not climate, rivers, or structures. |
| Ores | missing | ore cubes exist and do not generate |
| Structures | missing | 52 structure ids |
| Nether | missing | S26 |
| End | missing | S27 |
| Portals | missing | part of S26 for the nether portal |
| Advancements | missing | 1,866 advancement ids in the data dump. Not a near-term slice |
| Loot tables | missing | drops in S05 are hardcoded for three blocks, then a table |
| Commands | missing | out of scope until a server admin slice |

## Multiplayer

| System | Status | First slice |
| --- | --- | --- |
| Headless server | partial | `opencraft-server` binds 0.0.0.0:25570 and counts a 20 Hz loop. It does not simulate the voxel world yet. S12 |
| Protocol v1 | partial | Length-prefixed frames. Hello, account, join, block, inventory, and a block-edit snapshot. No section chunks and no Input simulation. S13 |
| Direct connect | partial | Direct Connection asks for address and port. The account form is not on that screen. S14 |
| Server list | partial | Saved servers live in `servers.txt`. Join uses the account you already signed in with. S15 |
| Authoritative edits | partial | Place and break are stored in `opencraft.db` and broadcast. The placing client also draws the edit immediately. S16 |
| Other players | missing | S17 |
| Account inventory | partial | Hotbar stacks and the selected slot reload from `opencraft.db` for that account. The creative grid is stored once the client has sent it. |
| Realms | missing | stays a coming-soon screen. Not a hosting target |
| Authentication | partial | Username and password hash, optional email, created in the client. No Microsoft accounts. HTTP signup is not this path. |

## Presentation

| System | Status | First slice |
| --- | --- | --- |
| Texture colors match the PNG | missing | S32 |
| Sky and fog | missing | S11 |
| Chunk shader | missing | S28 |
| Water shader | missing | S29 |
| Ray tracing | missing | S30, blocked |

Audio is a raylib device and a click sound. Music is not loaded. A sound slice can come any time after the tick exists. It is not on the critical path.
