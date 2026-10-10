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
| Inventory | partial | 9 hotbar slots and a creative 9x5 grid | S06 |
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
| Crafting | missing | S18, 2x2 and the crafting table, planks and sticks |
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
| Biomes | missing | S25, four biomes |
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
| Headless server | missing | S12 |
| Protocol v1 | missing | S13 |
| Direct connect | missing | S14 |
| Server list | missing | S15 |
| Authoritative edits | missing | S16 |
| Other players | missing | S17 |
| Realms | missing | stays a coming-soon screen. Not a hosting target |
| Authentication | missing | firewall or tunnel in v1. No Microsoft accounts |

## Presentation

| System | Status | First slice |
| --- | --- | --- |
| Sky and fog | missing | S11 |
| Chunk shader | missing | S28 |
| Water shader | missing | S29 |
| Ray tracing | missing | S30, blocked |

Audio is a raylib device and a click sound. Music is not loaded. A sound slice can come any time after the tick exists. It is not on the critical path.
