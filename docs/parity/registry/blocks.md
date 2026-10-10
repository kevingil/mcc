# Blocks

One line per Java Edition block id. Block states (facing, waterlogged, age) are not separate ids. Stairs and doors show up once, and the state work is a slice of its own.

IDs from the misode/mcmeta `summary` registries dump fetched 2026-10-10. That branch was on Minecraft Java Edition 26.4 Snapshot 3 (data version 5122, built 2026-10-06). Behavior target is the stable release Java Edition 26.3 (2026-09-15, protocol 777, data version 5023). A public 26.3 ID list counted 1,286 blocks, 1,658 items, and 161 entity types. This dump has a few more because it includes that snapshot.

Edit the status word only: `missing`, `cube`, `partial`, `done`. Regenerating this file keeps a `done` mark and any `partial` or `cube` mark that is not in the default set.

Status:

- `missing` means OpenCraft does not simulate it.
- `cube` means a full-cube stand-in exists in `client/voxel_types.h`. No states, drops, or hardness.
- `partial` means some behavior exists and the slice is not done.
- `done` means the owning slice's checks passed.

1288 ids. 0 done, 1 partial, 143 cube, 1144 missing.

## acacia

- `acacia_button` - missing
- `acacia_door` - missing
- `acacia_fence` - missing
- `acacia_fence_gate` - missing
- `acacia_hanging_sign` - missing
- `acacia_leaves` - cube
- `acacia_log` - cube
- `acacia_planks` - cube
- `acacia_pressure_plate` - missing
- `acacia_sapling` - missing
- `acacia_shelf` - missing
- `acacia_sign` - missing
- `acacia_slab` - missing
- `acacia_stairs` - missing
- `acacia_trapdoor` - missing
- `acacia_wall_hanging_sign` - missing
- `acacia_wall_sign` - missing
- `acacia_wood` - missing

## activator

- `activator_rail` - missing

## air

- `air` - missing

## allium

- `allium` - missing

## amethyst

- `amethyst_block` - missing
- `amethyst_cluster` - missing

## ancient

- `ancient_debris` - missing

## andesite

- `andesite` - cube
- `andesite_slab` - missing
- `andesite_stairs` - missing
- `andesite_wall` - missing

## anvil

- `anvil` - missing

## attached

- `attached_melon_stem` - missing
- `attached_pumpkin_stem` - missing

## azalea

- `azalea` - missing
- `azalea_leaves` - missing

## azure

- `azure_bluet` - missing

## bamboo

- `bamboo` - missing
- `bamboo_block` - missing
- `bamboo_button` - missing
- `bamboo_door` - missing
- `bamboo_fence` - missing
- `bamboo_fence_gate` - missing
- `bamboo_hanging_sign` - missing
- `bamboo_mosaic` - missing
- `bamboo_mosaic_slab` - missing
- `bamboo_mosaic_stairs` - missing
- `bamboo_planks` - missing
- `bamboo_pressure_plate` - missing
- `bamboo_sapling` - missing
- `bamboo_shelf` - missing
- `bamboo_sign` - missing
- `bamboo_slab` - missing
- `bamboo_stairs` - missing
- `bamboo_trapdoor` - missing
- `bamboo_wall_hanging_sign` - missing
- `bamboo_wall_sign` - missing

## banner

- `black_banner` - missing
- `blue_banner` - missing
- `brown_banner` - missing
- `cyan_banner` - missing
- `gray_banner` - missing
- `green_banner` - missing
- `light_blue_banner` - missing
- `light_gray_banner` - missing
- `lime_banner` - missing
- `magenta_banner` - missing
- `orange_banner` - missing
- `pink_banner` - missing
- `purple_banner` - missing
- `red_banner` - missing
- `white_banner` - missing
- `yellow_banner` - missing

## barrel

- `barrel` - missing

## barrier

- `barrier` - missing

## basalt

- `basalt` - missing

## beacon

- `beacon` - missing

## bed

- `black_bed` - missing
- `blue_bed` - missing
- `brown_bed` - missing
- `cyan_bed` - missing
- `gray_bed` - missing
- `green_bed` - missing
- `light_blue_bed` - missing
- `light_gray_bed` - missing
- `lime_bed` - missing
- `magenta_bed` - missing
- `orange_bed` - missing
- `pink_bed` - missing
- `purple_bed` - missing
- `red_bed` - missing
- `white_bed` - missing
- `yellow_bed` - missing

## bedrock

- `bedrock` - cube

## bee

- `bee_nest` - missing

## beehive

- `beehive` - missing

## beetroots

- `beetroots` - missing

## bell

- `bell` - missing

## big

- `big_dripleaf` - missing
- `big_dripleaf_stem` - missing

## birch

- `birch_button` - missing
- `birch_door` - missing
- `birch_fence` - missing
- `birch_fence_gate` - missing
- `birch_hanging_sign` - missing
- `birch_leaves` - cube
- `birch_log` - cube
- `birch_planks` - cube
- `birch_pressure_plate` - missing
- `birch_sapling` - missing
- `birch_shelf` - missing
- `birch_sign` - missing
- `birch_slab` - missing
- `birch_stairs` - missing
- `birch_trapdoor` - missing
- `birch_wall_hanging_sign` - missing
- `birch_wall_sign` - missing
- `birch_wood` - missing

## blackstone

- `blackstone` - missing
- `blackstone_slab` - missing
- `blackstone_stairs` - missing
- `blackstone_wall` - missing

## blast

- `blast_furnace` - missing

## bone

- `bone_block` - cube

## bookshelf

- `bookshelf` - cube

## brain

- `brain_coral` - missing
- `brain_coral_block` - missing
- `brain_coral_fan` - missing
- `brain_coral_wall_fan` - missing

## brewing

- `brewing_stand` - missing

## brick

- `brick_slab` - missing
- `brick_stairs` - missing
- `brick_wall` - missing

## bricks

- `bricks` - cube

## bubble

- `bubble_column` - missing
- `bubble_coral` - missing
- `bubble_coral_block` - missing
- `bubble_coral_fan` - missing
- `bubble_coral_wall_fan` - missing

## budding

- `budding_amethyst` - missing

## bush

- `bush` - missing

## cactus

- `cactus` - cube
- `cactus_flower` - missing

## cake

- `cake` - missing

## calcite

- `calcite` - missing

## calibrated

- `calibrated_sculk_sensor` - missing

## campfire

- `campfire` - missing

## candle

- `black_candle` - missing
- `blue_candle` - missing
- `brown_candle` - missing
- `candle` - missing
- `candle_cake` - missing
- `cyan_candle` - missing
- `gray_candle` - missing
- `green_candle` - missing
- `light_blue_candle` - missing
- `light_gray_candle` - missing
- `lime_candle` - missing
- `magenta_candle` - missing
- `orange_candle` - missing
- `pink_candle` - missing
- `purple_candle` - missing
- `red_candle` - missing
- `white_candle` - missing
- `yellow_candle` - missing

## candle_cake

- `black_candle_cake` - missing
- `blue_candle_cake` - missing
- `brown_candle_cake` - missing
- `cyan_candle_cake` - missing
- `gray_candle_cake` - missing
- `green_candle_cake` - missing
- `light_blue_candle_cake` - missing
- `light_gray_candle_cake` - missing
- `lime_candle_cake` - missing
- `magenta_candle_cake` - missing
- `orange_candle_cake` - missing
- `pink_candle_cake` - missing
- `purple_candle_cake` - missing
- `red_candle_cake` - missing
- `white_candle_cake` - missing
- `yellow_candle_cake` - missing

## carpet

- `black_carpet` - missing
- `blue_carpet` - missing
- `brown_carpet` - missing
- `cyan_carpet` - missing
- `gray_carpet` - missing
- `green_carpet` - missing
- `light_blue_carpet` - missing
- `light_gray_carpet` - missing
- `lime_carpet` - missing
- `magenta_carpet` - missing
- `orange_carpet` - missing
- `pink_carpet` - missing
- `purple_carpet` - missing
- `red_carpet` - missing
- `white_carpet` - missing
- `yellow_carpet` - missing

## carrots

- `carrots` - missing

## cartography

- `cartography_table` - missing

## carved

- `carved_pumpkin` - missing

## cauldron

- `cauldron` - missing

## cave

- `cave_air` - missing
- `cave_vines` - missing
- `cave_vines_plant` - missing

## chain

- `chain_command_block` - missing

## cherry

- `cherry_button` - missing
- `cherry_door` - missing
- `cherry_fence` - missing
- `cherry_fence_gate` - missing
- `cherry_hanging_sign` - missing
- `cherry_leaves` - missing
- `cherry_log` - missing
- `cherry_planks` - missing
- `cherry_pressure_plate` - missing
- `cherry_sapling` - missing
- `cherry_shelf` - missing
- `cherry_sign` - missing
- `cherry_slab` - missing
- `cherry_stairs` - missing
- `cherry_trapdoor` - missing
- `cherry_wall_hanging_sign` - missing
- `cherry_wall_sign` - missing
- `cherry_wood` - missing

## chest

- `chest` - cube

## chipped

- `chipped_anvil` - missing

## chiseled

- `chiseled_bookshelf` - missing
- `chiseled_cinnabar` - missing
- `chiseled_copper` - missing
- `chiseled_deepslate` - missing
- `chiseled_nether_bricks` - missing
- `chiseled_polished_blackstone` - missing
- `chiseled_quartz_block` - cube
- `chiseled_red_sandstone` - missing
- `chiseled_resin_bricks` - missing
- `chiseled_sandstone` - cube
- `chiseled_stone_bricks` - missing
- `chiseled_sulfur` - missing
- `chiseled_tuff` - missing
- `chiseled_tuff_bricks` - missing

## chorus

- `chorus_flower` - missing
- `chorus_plant` - missing

## cinnabar

- `cinnabar` - missing
- `cinnabar_brick_slab` - missing
- `cinnabar_brick_stairs` - missing
- `cinnabar_brick_wall` - missing
- `cinnabar_bricks` - missing
- `cinnabar_slab` - missing
- `cinnabar_stairs` - missing
- `cinnabar_wall` - missing

## clay

- `clay` - cube

## closed

- `closed_eyeblossom` - missing

## coal

- `coal_block` - cube
- `coal_ore` - cube

## coarse

- `coarse_dirt` - missing

## cobbled

- `cobbled_deepslate` - missing
- `cobbled_deepslate_slab` - missing
- `cobbled_deepslate_stairs` - missing
- `cobbled_deepslate_wall` - missing

## cobblestone

- `cobblestone` - cube
- `cobblestone_slab` - missing
- `cobblestone_stairs` - missing
- `cobblestone_wall` - missing

## cobweb

- `cobweb` - missing

## cocoa

- `cocoa` - missing

## command

- `command_block` - missing

## comparator

- `comparator` - missing

## composter

- `composter` - missing

## concrete

- `black_concrete` - cube
- `blue_concrete` - cube
- `brown_concrete` - cube
- `cyan_concrete` - cube
- `gray_concrete` - cube
- `green_concrete` - cube
- `light_blue_concrete` - cube
- `light_gray_concrete` - cube
- `lime_concrete` - cube
- `magenta_concrete` - cube
- `orange_concrete` - cube
- `pink_concrete` - cube
- `purple_concrete` - cube
- `red_concrete` - cube
- `white_concrete` - cube
- `yellow_concrete` - cube

## concrete_powder

- `black_concrete_powder` - missing
- `blue_concrete_powder` - missing
- `brown_concrete_powder` - missing
- `cyan_concrete_powder` - missing
- `gray_concrete_powder` - missing
- `green_concrete_powder` - missing
- `light_blue_concrete_powder` - missing
- `light_gray_concrete_powder` - missing
- `lime_concrete_powder` - missing
- `magenta_concrete_powder` - missing
- `orange_concrete_powder` - missing
- `pink_concrete_powder` - missing
- `purple_concrete_powder` - missing
- `red_concrete_powder` - missing
- `white_concrete_powder` - missing
- `yellow_concrete_powder` - missing

## concrete_slab

- `black_concrete_slab` - missing
- `blue_concrete_slab` - missing
- `brown_concrete_slab` - missing
- `cyan_concrete_slab` - missing
- `gray_concrete_slab` - missing
- `green_concrete_slab` - missing
- `light_blue_concrete_slab` - missing
- `light_gray_concrete_slab` - missing
- `lime_concrete_slab` - missing
- `magenta_concrete_slab` - missing
- `orange_concrete_slab` - missing
- `pink_concrete_slab` - missing
- `purple_concrete_slab` - missing
- `red_concrete_slab` - missing
- `white_concrete_slab` - missing
- `yellow_concrete_slab` - missing

## concrete_stairs

- `black_concrete_stairs` - missing
- `blue_concrete_stairs` - missing
- `brown_concrete_stairs` - missing
- `cyan_concrete_stairs` - missing
- `gray_concrete_stairs` - missing
- `green_concrete_stairs` - missing
- `light_blue_concrete_stairs` - missing
- `light_gray_concrete_stairs` - missing
- `lime_concrete_stairs` - missing
- `magenta_concrete_stairs` - missing
- `orange_concrete_stairs` - missing
- `pink_concrete_stairs` - missing
- `purple_concrete_stairs` - missing
- `red_concrete_stairs` - missing
- `white_concrete_stairs` - missing
- `yellow_concrete_stairs` - missing

## conduit

- `conduit` - missing

## copper

- `copper_bars` - missing
- `copper_block` - missing
- `copper_bulb` - missing
- `copper_chain` - missing
- `copper_chest` - missing
- `copper_door` - missing
- `copper_golem_statue` - missing
- `copper_grate` - missing
- `copper_lantern` - missing
- `copper_ore` - missing
- `copper_torch` - missing
- `copper_trapdoor` - missing
- `copper_wall_torch` - missing

## cornflower

- `cornflower` - missing

## cracked

- `cracked_deepslate_bricks` - missing
- `cracked_deepslate_tiles` - missing
- `cracked_nether_bricks` - missing
- `cracked_polished_blackstone_bricks` - missing
- `cracked_stone_bricks` - cube

## crafter

- `crafter` - missing

## crafting

- `crafting_table` - cube

## creaking

- `creaking_heart` - missing

## creeper

- `creeper_head` - missing
- `creeper_wall_head` - missing

## crimson

- `crimson_button` - missing
- `crimson_door` - missing
- `crimson_fence` - missing
- `crimson_fence_gate` - missing
- `crimson_fungus` - missing
- `crimson_hanging_sign` - missing
- `crimson_hyphae` - missing
- `crimson_nylium` - missing
- `crimson_planks` - missing
- `crimson_pressure_plate` - missing
- `crimson_roots` - missing
- `crimson_shelf` - missing
- `crimson_sign` - missing
- `crimson_slab` - missing
- `crimson_stairs` - missing
- `crimson_stem` - missing
- `crimson_trapdoor` - missing
- `crimson_wall_hanging_sign` - missing
- `crimson_wall_sign` - missing

## crying

- `crying_obsidian` - missing

## cut

- `cut_copper` - missing
- `cut_copper_slab` - missing
- `cut_copper_stairs` - missing
- `cut_red_sandstone` - missing
- `cut_red_sandstone_slab` - missing
- `cut_sandstone` - cube
- `cut_sandstone_slab` - missing

## damaged

- `damaged_anvil` - missing

## dandelion

- `dandelion` - missing

## dark

- `dark_prismarine` - missing
- `dark_prismarine_slab` - missing
- `dark_prismarine_stairs` - missing

## dark_oak

- `dark_oak_button` - missing
- `dark_oak_door` - missing
- `dark_oak_fence` - missing
- `dark_oak_fence_gate` - missing
- `dark_oak_hanging_sign` - missing
- `dark_oak_leaves` - cube
- `dark_oak_log` - cube
- `dark_oak_planks` - cube
- `dark_oak_pressure_plate` - missing
- `dark_oak_sapling` - missing
- `dark_oak_shelf` - missing
- `dark_oak_sign` - missing
- `dark_oak_slab` - missing
- `dark_oak_stairs` - missing
- `dark_oak_trapdoor` - missing
- `dark_oak_wall_hanging_sign` - missing
- `dark_oak_wall_sign` - missing
- `dark_oak_wood` - missing

## daylight

- `daylight_detector` - missing

## dead

- `dead_brain_coral` - missing
- `dead_brain_coral_block` - missing
- `dead_brain_coral_fan` - missing
- `dead_brain_coral_wall_fan` - missing
- `dead_bubble_coral` - missing
- `dead_bubble_coral_block` - missing
- `dead_bubble_coral_fan` - missing
- `dead_bubble_coral_wall_fan` - missing
- `dead_bush` - missing
- `dead_fire_coral` - missing
- `dead_fire_coral_block` - missing
- `dead_fire_coral_fan` - missing
- `dead_fire_coral_wall_fan` - missing
- `dead_horn_coral` - missing
- `dead_horn_coral_block` - missing
- `dead_horn_coral_fan` - missing
- `dead_horn_coral_wall_fan` - missing
- `dead_tube_coral` - missing
- `dead_tube_coral_block` - missing
- `dead_tube_coral_fan` - missing
- `dead_tube_coral_wall_fan` - missing

## decorated

- `decorated_pot` - missing

## deepslate

- `deepslate` - missing
- `deepslate_brick_slab` - missing
- `deepslate_brick_stairs` - missing
- `deepslate_brick_wall` - missing
- `deepslate_bricks` - missing
- `deepslate_coal_ore` - missing
- `deepslate_copper_ore` - missing
- `deepslate_diamond_ore` - missing
- `deepslate_emerald_ore` - missing
- `deepslate_gold_ore` - missing
- `deepslate_iron_ore` - missing
- `deepslate_lapis_ore` - missing
- `deepslate_redstone_ore` - missing
- `deepslate_tile_slab` - missing
- `deepslate_tile_stairs` - missing
- `deepslate_tile_wall` - missing
- `deepslate_tiles` - missing

## detector

- `detector_rail` - missing

## diamond

- `diamond_block` - cube
- `diamond_ore` - cube

## diorite

- `diorite` - cube
- `diorite_slab` - missing
- `diorite_stairs` - missing
- `diorite_wall` - missing

## dirt

- `dirt` - cube
- `dirt_path` - missing

## dispenser

- `dispenser` - missing

## dragon

- `dragon_egg` - missing
- `dragon_head` - missing
- `dragon_wall_head` - missing

## dried

- `dried_ghast` - missing
- `dried_kelp_block` - missing

## dripstone

- `dripstone_block` - missing

## dropper

- `dropper` - missing

## emerald

- `emerald_block` - cube
- `emerald_ore` - cube

## enchanting

- `enchanting_table` - missing

## end

- `end_gateway` - missing
- `end_portal` - missing
- `end_portal_frame` - missing
- `end_rod` - missing
- `end_stone` - cube
- `end_stone_brick_slab` - missing
- `end_stone_brick_stairs` - missing
- `end_stone_brick_wall` - missing
- `end_stone_bricks` - missing

## ender

- `ender_chest` - missing

## exposed

- `exposed_chiseled_copper` - missing
- `exposed_copper` - missing
- `exposed_copper_bars` - missing
- `exposed_copper_bulb` - missing
- `exposed_copper_chain` - missing
- `exposed_copper_chest` - missing
- `exposed_copper_door` - missing
- `exposed_copper_golem_statue` - missing
- `exposed_copper_grate` - missing
- `exposed_copper_lantern` - missing
- `exposed_copper_trapdoor` - missing
- `exposed_cut_copper` - missing
- `exposed_cut_copper_slab` - missing
- `exposed_cut_copper_stairs` - missing
- `exposed_lightning_rod` - missing

## farmland

- `farmland` - missing

## fern

- `fern` - missing

## fire

- `fire` - missing
- `fire_coral` - missing
- `fire_coral_block` - missing
- `fire_coral_fan` - missing
- `fire_coral_wall_fan` - missing

## firefly

- `firefly_bush` - missing

## fletching

- `fletching_table` - missing

## flower

- `flower_pot` - missing

## flowering

- `flowering_azalea` - missing
- `flowering_azalea_leaves` - missing

## frogspawn

- `frogspawn` - missing

## frosted

- `frosted_ice` - missing

## furnace

- `furnace` - cube

## gilded

- `gilded_blackstone` - missing

## glass

- `glass` - cube
- `glass_pane` - missing

## glazed_terracotta

- `black_glazed_terracotta` - missing
- `blue_glazed_terracotta` - missing
- `brown_glazed_terracotta` - missing
- `cyan_glazed_terracotta` - missing
- `gray_glazed_terracotta` - missing
- `green_glazed_terracotta` - missing
- `light_blue_glazed_terracotta` - missing
- `light_gray_glazed_terracotta` - missing
- `lime_glazed_terracotta` - missing
- `magenta_glazed_terracotta` - missing
- `orange_glazed_terracotta` - missing
- `pink_glazed_terracotta` - missing
- `purple_glazed_terracotta` - missing
- `red_glazed_terracotta` - missing
- `white_glazed_terracotta` - missing
- `yellow_glazed_terracotta` - missing

## glow

- `glow_lichen` - missing

## glowstone

- `glowstone` - cube

## gold

- `gold_block` - cube
- `gold_ore` - cube

## golden

- `golden_dandelion` - missing

## granite

- `granite` - cube
- `granite_slab` - missing
- `granite_stairs` - missing
- `granite_wall` - missing

## grass

- `grass_block` - cube

## gravel

- `gravel` - cube

## grindstone

- `grindstone` - missing

## hanging

- `hanging_roots` - missing

## hay

- `hay_block` - cube

## heavy

- `heavy_core` - missing
- `heavy_weighted_pressure_plate` - missing

## honey

- `honey_block` - missing

## honeycomb

- `honeycomb_block` - cube

## hopper

- `hopper` - missing

## horn

- `horn_coral` - missing
- `horn_coral_block` - missing
- `horn_coral_fan` - missing
- `horn_coral_wall_fan` - missing

## ice

- `blue_ice` - cube
- `ice` - cube
- `ice_crystal` - missing

## icicle

- `icicle` - missing

## infested

- `infested_chiseled_stone_bricks` - missing
- `infested_cobblestone` - missing
- `infested_cracked_stone_bricks` - missing
- `infested_deepslate` - missing
- `infested_mossy_stone_bricks` - missing
- `infested_stone` - missing
- `infested_stone_bricks` - missing

## iron

- `iron_bars` - missing
- `iron_block` - cube
- `iron_chain` - missing
- `iron_door` - missing
- `iron_ore` - cube
- `iron_trapdoor` - missing

## jack

- `jack_o_lantern` - cube

## jigsaw

- `jigsaw` - missing

## jukebox

- `jukebox` - missing

## jungle

- `jungle_button` - missing
- `jungle_door` - missing
- `jungle_fence` - missing
- `jungle_fence_gate` - missing
- `jungle_hanging_sign` - missing
- `jungle_leaves` - missing
- `jungle_log` - missing
- `jungle_planks` - missing
- `jungle_pressure_plate` - missing
- `jungle_sapling` - missing
- `jungle_shelf` - missing
- `jungle_sign` - missing
- `jungle_slab` - missing
- `jungle_stairs` - missing
- `jungle_trapdoor` - missing
- `jungle_wall_hanging_sign` - missing
- `jungle_wall_sign` - missing
- `jungle_wood` - missing

## kelp

- `kelp` - missing
- `kelp_plant` - missing

## ladder

- `ladder` - missing

## lantern

- `lantern` - missing

## lapis

- `lapis_block` - cube
- `lapis_ore` - cube

## large

- `large_amethyst_bud` - missing
- `large_fern` - missing

## lava

- `lava` - missing
- `lava_cauldron` - missing

## leaf

- `leaf_litter` - missing

## lectern

- `lectern` - missing

## lever

- `lever` - missing

## light

- `light` - missing
- `light_weighted_pressure_plate` - missing

## lightning

- `lightning_rod` - missing

## lilac

- `lilac` - missing

## lily

- `lily_of_the_valley` - missing
- `lily_pad` - missing

## lodestone

- `lodestone` - missing

## loom

- `loom` - missing

## magma

- `magma_block` - cube

## mangrove

- `mangrove_button` - missing
- `mangrove_door` - missing
- `mangrove_fence` - missing
- `mangrove_fence_gate` - missing
- `mangrove_hanging_sign` - missing
- `mangrove_leaves` - missing
- `mangrove_log` - missing
- `mangrove_planks` - missing
- `mangrove_pressure_plate` - missing
- `mangrove_propagule` - missing
- `mangrove_roots` - missing
- `mangrove_shelf` - missing
- `mangrove_sign` - missing
- `mangrove_slab` - missing
- `mangrove_stairs` - missing
- `mangrove_trapdoor` - missing
- `mangrove_wall_hanging_sign` - missing
- `mangrove_wall_sign` - missing
- `mangrove_wood` - missing

## medium

- `medium_amethyst_bud` - missing

## melon

- `melon` - cube
- `melon_stem` - missing

## moss

- `moss_block` - missing
- `moss_carpet` - missing

## mossy

- `mossy_cobblestone` - cube
- `mossy_cobblestone_slab` - missing
- `mossy_cobblestone_stairs` - missing
- `mossy_cobblestone_wall` - missing
- `mossy_stone_brick_slab` - missing
- `mossy_stone_brick_stairs` - missing
- `mossy_stone_brick_wall` - missing
- `mossy_stone_bricks` - cube

## moving

- `moving_piston` - missing

## mud

- `mud` - missing
- `mud_brick_slab` - missing
- `mud_brick_stairs` - missing
- `mud_brick_wall` - missing
- `mud_bricks` - missing

## muddy

- `muddy_mangrove_roots` - missing

## mushroom

- `brown_mushroom` - missing
- `mushroom_stem` - missing
- `red_mushroom` - missing

## mushroom_block

- `brown_mushroom_block` - missing
- `red_mushroom_block` - missing

## mycelium

- `mycelium` - missing

## nether

- `nether_brick_fence` - missing
- `nether_brick_slab` - missing
- `nether_brick_stairs` - missing
- `nether_brick_wall` - missing
- `nether_bricks` - missing
- `nether_gold_ore` - missing
- `nether_portal` - missing
- `nether_quartz_ore` - missing
- `nether_sprouts` - missing
- `nether_wart` - missing
- `nether_wart_block` - missing

## nether_brick_slab

- `red_nether_brick_slab` - missing

## nether_brick_stairs

- `red_nether_brick_stairs` - missing

## nether_brick_wall

- `red_nether_brick_wall` - missing

## nether_bricks

- `red_nether_bricks` - missing

## netherite

- `netherite_block` - missing

## netherrack

- `netherrack` - cube

## note

- `note_block` - missing

## oak

- `oak_button` - missing
- `oak_door` - missing
- `oak_fence` - missing
- `oak_fence_gate` - missing
- `oak_hanging_sign` - missing
- `oak_leaves` - cube
- `oak_log` - cube
- `oak_planks` - cube
- `oak_pressure_plate` - missing
- `oak_sapling` - missing
- `oak_shelf` - missing
- `oak_sign` - missing
- `oak_slab` - missing
- `oak_stairs` - missing
- `oak_trapdoor` - missing
- `oak_wall_hanging_sign` - missing
- `oak_wall_sign` - missing
- `oak_wood` - missing

## observer

- `observer` - missing

## obsidian

- `obsidian` - cube

## ochre

- `ochre_froglight` - missing

## open

- `open_eyeblossom` - missing

## orchid

- `blue_orchid` - missing

## oxeye

- `oxeye_daisy` - missing

## oxidized

- `oxidized_chiseled_copper` - missing
- `oxidized_copper` - missing
- `oxidized_copper_bars` - missing
- `oxidized_copper_bulb` - missing
- `oxidized_copper_chain` - missing
- `oxidized_copper_chest` - missing
- `oxidized_copper_door` - missing
- `oxidized_copper_golem_statue` - missing
- `oxidized_copper_grate` - missing
- `oxidized_copper_lantern` - missing
- `oxidized_copper_trapdoor` - missing
- `oxidized_cut_copper` - missing
- `oxidized_cut_copper_slab` - missing
- `oxidized_cut_copper_stairs` - missing
- `oxidized_lightning_rod` - missing

## packed

- `packed_ice` - cube
- `packed_mud` - missing

## pale

- `pale_hanging_moss` - missing
- `pale_moss_block` - missing
- `pale_moss_carpet` - missing

## pale_oak

- `pale_oak_button` - missing
- `pale_oak_door` - missing
- `pale_oak_fence` - missing
- `pale_oak_fence_gate` - missing
- `pale_oak_hanging_sign` - missing
- `pale_oak_leaves` - missing
- `pale_oak_log` - missing
- `pale_oak_planks` - missing
- `pale_oak_pressure_plate` - missing
- `pale_oak_sapling` - missing
- `pale_oak_shelf` - missing
- `pale_oak_sign` - missing
- `pale_oak_slab` - missing
- `pale_oak_stairs` - missing
- `pale_oak_trapdoor` - missing
- `pale_oak_wall_hanging_sign` - missing
- `pale_oak_wall_sign` - missing
- `pale_oak_wood` - missing

## pearlescent

- `pearlescent_froglight` - missing

## peony

- `peony` - missing

## petals

- `pink_petals` - missing

## petrified

- `petrified_oak_slab` - missing

## piglin

- `piglin_head` - missing
- `piglin_wall_head` - missing

## piston

- `piston` - missing
- `piston_head` - missing

## pitcher

- `pitcher_crop` - missing
- `pitcher_plant` - missing

## player

- `player_head` - missing
- `player_wall_head` - missing

## podzol

- `podzol` - missing

## pointed

- `pointed_dripstone` - missing

## polished

- `polished_andesite` - missing
- `polished_andesite_slab` - missing
- `polished_andesite_stairs` - missing
- `polished_basalt` - missing
- `polished_blackstone` - missing
- `polished_blackstone_brick_slab` - missing
- `polished_blackstone_brick_stairs` - missing
- `polished_blackstone_brick_wall` - missing
- `polished_blackstone_bricks` - missing
- `polished_blackstone_button` - missing
- `polished_blackstone_pressure_plate` - missing
- `polished_blackstone_slab` - missing
- `polished_blackstone_stairs` - missing
- `polished_blackstone_wall` - missing
- `polished_cinnabar` - missing
- `polished_cinnabar_slab` - missing
- `polished_cinnabar_stairs` - missing
- `polished_cinnabar_wall` - missing
- `polished_deepslate` - missing
- `polished_deepslate_slab` - missing
- `polished_deepslate_stairs` - missing
- `polished_deepslate_wall` - missing
- `polished_diorite` - missing
- `polished_diorite_slab` - missing
- `polished_diorite_stairs` - missing
- `polished_granite` - missing
- `polished_granite_slab` - missing
- `polished_granite_stairs` - missing
- `polished_sulfur` - missing
- `polished_sulfur_slab` - missing
- `polished_sulfur_stairs` - missing
- `polished_sulfur_wall` - missing
- `polished_tuff` - missing
- `polished_tuff_slab` - missing
- `polished_tuff_stairs` - missing
- `polished_tuff_wall` - missing

## poplar

- `poplar_button` - missing
- `poplar_door` - missing
- `poplar_fence` - missing
- `poplar_fence_gate` - missing
- `poplar_hanging_sign` - missing
- `poplar_log` - missing
- `poplar_planks` - missing
- `poplar_pressure_plate` - missing
- `poplar_sapling` - missing
- `poplar_shelf` - missing
- `poplar_sign` - missing
- `poplar_slab` - missing
- `poplar_stairs` - missing
- `poplar_trapdoor` - missing
- `poplar_wall_hanging_sign` - missing
- `poplar_wall_sign` - missing
- `poplar_wood` - missing

## poplar_leaves

- `orange_poplar_leaves` - missing
- `red_poplar_leaves` - missing
- `yellow_poplar_leaves` - missing

## poppy

- `poppy` - missing

## potatoes

- `potatoes` - missing

## potent

- `potent_sulfur` - missing

## potted

- `potted_acacia_sapling` - missing
- `potted_allium` - missing
- `potted_azalea_bush` - missing
- `potted_azure_bluet` - missing
- `potted_bamboo` - missing
- `potted_birch_sapling` - missing
- `potted_blue_orchid` - missing
- `potted_brown_mushroom` - missing
- `potted_cactus` - missing
- `potted_cherry_sapling` - missing
- `potted_closed_eyeblossom` - missing
- `potted_cornflower` - missing
- `potted_crimson_fungus` - missing
- `potted_crimson_roots` - missing
- `potted_dandelion` - missing
- `potted_dark_oak_sapling` - missing
- `potted_dead_bush` - missing
- `potted_fern` - missing
- `potted_flowering_azalea_bush` - missing
- `potted_golden_dandelion` - missing
- `potted_jungle_sapling` - missing
- `potted_lily_of_the_valley` - missing
- `potted_mangrove_propagule` - missing
- `potted_oak_sapling` - missing
- `potted_open_eyeblossom` - missing
- `potted_orange_tulip` - missing
- `potted_oxeye_daisy` - missing
- `potted_pale_oak_sapling` - missing
- `potted_pink_tulip` - missing
- `potted_poplar_sapling` - missing
- `potted_poppy` - missing
- `potted_red_mushroom` - missing
- `potted_red_tulip` - missing
- `potted_spruce_sapling` - missing
- `potted_torchflower` - missing
- `potted_warped_fungus` - missing
- `potted_warped_roots` - missing
- `potted_white_tulip` - missing
- `potted_wither_rose` - missing

## powder

- `powder_snow` - missing
- `powder_snow_cauldron` - missing

## powered

- `powered_rail` - missing

## prismarine

- `prismarine` - cube
- `prismarine_brick_slab` - missing
- `prismarine_brick_stairs` - missing
- `prismarine_bricks` - missing
- `prismarine_slab` - missing
- `prismarine_stairs` - missing
- `prismarine_wall` - missing

## pumpkin

- `pumpkin` - cube
- `pumpkin_stem` - missing

## purpur

- `purpur_block` - cube
- `purpur_pillar` - missing
- `purpur_slab` - missing
- `purpur_stairs` - missing

## quartz

- `quartz_block` - cube
- `quartz_bricks` - missing
- `quartz_pillar` - cube
- `quartz_slab` - missing
- `quartz_stairs` - missing

## rail

- `rail` - missing

## raw

- `raw_copper_block` - missing
- `raw_gold_block` - missing
- `raw_iron_block` - missing

## redstone

- `redstone_block` - cube
- `redstone_lamp` - missing
- `redstone_ore` - cube
- `redstone_torch` - missing
- `redstone_wall_torch` - missing
- `redstone_wire` - missing

## reinforced

- `reinforced_deepslate` - missing

## repeater

- `repeater` - missing

## repeating

- `repeating_command_block` - missing

## resin

- `resin_block` - missing
- `resin_brick_slab` - missing
- `resin_brick_stairs` - missing
- `resin_brick_wall` - missing
- `resin_bricks` - missing
- `resin_clump` - missing

## respawn

- `respawn_anchor` - missing

## rooted

- `rooted_dirt` - missing

## rose

- `rose_bush` - missing

## sand

- `red_sand` - cube
- `sand` - cube

## sandstone

- `red_sandstone` - cube
- `sandstone` - cube
- `sandstone_slab` - missing
- `sandstone_stairs` - missing
- `sandstone_wall` - missing

## sandstone_slab

- `red_sandstone_slab` - missing

## sandstone_stairs

- `red_sandstone_stairs` - missing

## sandstone_wall

- `red_sandstone_wall` - missing

## scaffolding

- `scaffolding` - missing

## sculk

- `sculk` - missing
- `sculk_catalyst` - missing
- `sculk_sensor` - missing
- `sculk_shrieker` - missing
- `sculk_vein` - missing

## sea

- `sea_lantern` - cube
- `sea_pickle` - missing

## seagrass

- `seagrass` - missing

## shelf

- `shelf_mushroom` - missing

## short

- `short_dry_grass` - missing
- `short_grass` - missing

## shroomlight

- `shroomlight` - missing

## shrub

- `red_shrub` - missing

## shulker

- `shulker_box` - missing

## shulker_box

- `black_shulker_box` - missing
- `blue_shulker_box` - missing
- `brown_shulker_box` - missing
- `cyan_shulker_box` - missing
- `gray_shulker_box` - missing
- `green_shulker_box` - missing
- `light_blue_shulker_box` - missing
- `light_gray_shulker_box` - missing
- `lime_shulker_box` - missing
- `magenta_shulker_box` - missing
- `orange_shulker_box` - missing
- `pink_shulker_box` - missing
- `purple_shulker_box` - missing
- `red_shulker_box` - missing
- `white_shulker_box` - missing
- `yellow_shulker_box` - missing

## skeleton

- `skeleton_skull` - missing
- `skeleton_wall_skull` - missing

## slime

- `slime_block` - missing

## small

- `small_amethyst_bud` - missing
- `small_dripleaf` - missing

## smithing

- `smithing_table` - missing

## smoker

- `smoker` - missing

## smooth

- `smooth_basalt` - missing
- `smooth_quartz` - missing
- `smooth_quartz_slab` - missing
- `smooth_quartz_stairs` - missing
- `smooth_red_sandstone` - missing
- `smooth_red_sandstone_slab` - missing
- `smooth_red_sandstone_stairs` - missing
- `smooth_sandstone` - missing
- `smooth_sandstone_slab` - missing
- `smooth_sandstone_stairs` - missing
- `smooth_stone` - cube
- `smooth_stone_slab` - missing

## sniffer

- `sniffer_egg` - missing

## snow

- `snow` - missing
- `snow_block` - cube

## soul

- `soul_campfire` - missing
- `soul_fire` - missing
- `soul_lantern` - missing
- `soul_sand` - cube
- `soul_soil` - missing
- `soul_torch` - missing
- `soul_wall_torch` - missing

## spawner

- `spawner` - missing

## sponge

- `sponge` - cube

## spore

- `spore_blossom` - missing

## spruce

- `spruce_button` - missing
- `spruce_door` - missing
- `spruce_fence` - missing
- `spruce_fence_gate` - missing
- `spruce_hanging_sign` - missing
- `spruce_leaves` - missing
- `spruce_log` - missing
- `spruce_planks` - missing
- `spruce_pressure_plate` - missing
- `spruce_sapling` - missing
- `spruce_shelf` - missing
- `spruce_sign` - missing
- `spruce_slab` - missing
- `spruce_stairs` - missing
- `spruce_trapdoor` - missing
- `spruce_wall_hanging_sign` - missing
- `spruce_wall_sign` - missing
- `spruce_wood` - missing

## stained_glass

- `black_stained_glass` - cube
- `blue_stained_glass` - cube
- `brown_stained_glass` - cube
- `cyan_stained_glass` - cube
- `gray_stained_glass` - cube
- `green_stained_glass` - cube
- `light_blue_stained_glass` - cube
- `light_gray_stained_glass` - cube
- `lime_stained_glass` - cube
- `magenta_stained_glass` - cube
- `orange_stained_glass` - cube
- `pink_stained_glass` - cube
- `purple_stained_glass` - cube
- `red_stained_glass` - cube
- `white_stained_glass` - cube
- `yellow_stained_glass` - cube

## stained_glass_pane

- `black_stained_glass_pane` - missing
- `blue_stained_glass_pane` - missing
- `brown_stained_glass_pane` - missing
- `cyan_stained_glass_pane` - missing
- `gray_stained_glass_pane` - missing
- `green_stained_glass_pane` - missing
- `light_blue_stained_glass_pane` - missing
- `light_gray_stained_glass_pane` - missing
- `lime_stained_glass_pane` - missing
- `magenta_stained_glass_pane` - missing
- `orange_stained_glass_pane` - missing
- `pink_stained_glass_pane` - missing
- `purple_stained_glass_pane` - missing
- `red_stained_glass_pane` - missing
- `white_stained_glass_pane` - missing
- `yellow_stained_glass_pane` - missing

## sticky

- `sticky_piston` - missing

## stone

- `stone` - cube
- `stone_brick_slab` - missing
- `stone_brick_stairs` - missing
- `stone_brick_wall` - missing
- `stone_bricks` - cube
- `stone_button` - missing
- `stone_pressure_plate` - missing
- `stone_slab` - missing
- `stone_stairs` - missing

## stonecutter

- `stonecutter` - missing

## straw

- `straw_bed` - missing

## stripped

- `stripped_acacia_log` - missing
- `stripped_acacia_wood` - missing
- `stripped_bamboo_block` - missing
- `stripped_birch_log` - missing
- `stripped_birch_wood` - missing
- `stripped_cherry_log` - missing
- `stripped_cherry_wood` - missing
- `stripped_crimson_hyphae` - missing
- `stripped_crimson_stem` - missing
- `stripped_dark_oak_log` - missing
- `stripped_dark_oak_wood` - missing
- `stripped_jungle_log` - missing
- `stripped_jungle_wood` - missing
- `stripped_mangrove_log` - missing
- `stripped_mangrove_wood` - missing
- `stripped_oak_log` - missing
- `stripped_oak_wood` - missing
- `stripped_pale_oak_log` - missing
- `stripped_pale_oak_wood` - missing
- `stripped_poplar_log` - missing
- `stripped_poplar_wood` - missing
- `stripped_spruce_log` - missing
- `stripped_spruce_wood` - missing
- `stripped_warped_hyphae` - missing
- `stripped_warped_stem` - missing

## structure

- `structure_block` - missing
- `structure_void` - missing

## sugar

- `sugar_cane` - missing

## sulfur

- `sulfur` - missing
- `sulfur_brick_slab` - missing
- `sulfur_brick_stairs` - missing
- `sulfur_brick_wall` - missing
- `sulfur_bricks` - missing
- `sulfur_slab` - missing
- `sulfur_spike` - missing
- `sulfur_stairs` - missing
- `sulfur_wall` - missing

## sunflower

- `sunflower` - missing

## suspicious

- `suspicious_gravel` - missing
- `suspicious_sand` - missing

## sweet

- `sweet_berry_bush` - missing

## tall

- `tall_dry_grass` - missing
- `tall_grass` - missing
- `tall_seagrass` - missing

## target

- `target` - missing

## terracotta

- `black_terracotta` - cube
- `blue_terracotta` - cube
- `brown_terracotta` - cube
- `cyan_terracotta` - cube
- `gray_terracotta` - cube
- `green_terracotta` - cube
- `light_blue_terracotta` - cube
- `light_gray_terracotta` - cube
- `lime_terracotta` - cube
- `magenta_terracotta` - cube
- `orange_terracotta` - cube
- `pink_terracotta` - cube
- `purple_terracotta` - cube
- `red_terracotta` - cube
- `terracotta` - cube
- `white_terracotta` - cube
- `yellow_terracotta` - cube

## test

- `test_block` - missing
- `test_instance_block` - missing

## tinted

- `tinted_glass` - missing

## tnt

- `tnt` - missing

## torch

- `torch` - missing

## torchflower

- `torchflower` - missing
- `torchflower_crop` - missing

## trapped

- `trapped_chest` - missing

## trial

- `trial_spawner` - missing

## tripwire

- `tripwire` - missing
- `tripwire_hook` - missing

## tube

- `tube_coral` - missing
- `tube_coral_block` - missing
- `tube_coral_fan` - missing
- `tube_coral_wall_fan` - missing

## tuff

- `tuff` - missing
- `tuff_brick_slab` - missing
- `tuff_brick_stairs` - missing
- `tuff_brick_wall` - missing
- `tuff_bricks` - missing
- `tuff_slab` - missing
- `tuff_stairs` - missing
- `tuff_wall` - missing

## tulip

- `orange_tulip` - missing
- `pink_tulip` - missing
- `red_tulip` - missing
- `white_tulip` - missing

## turtle

- `turtle_egg` - missing

## twisting

- `twisting_vines` - missing
- `twisting_vines_plant` - missing

## vault

- `vault` - missing

## verdant

- `verdant_froglight` - missing

## vine

- `vine` - missing

## void

- `void_air` - missing

## wall

- `wall_torch` - missing

## wall_banner

- `black_wall_banner` - missing
- `blue_wall_banner` - missing
- `brown_wall_banner` - missing
- `cyan_wall_banner` - missing
- `gray_wall_banner` - missing
- `green_wall_banner` - missing
- `light_blue_wall_banner` - missing
- `light_gray_wall_banner` - missing
- `lime_wall_banner` - missing
- `magenta_wall_banner` - missing
- `orange_wall_banner` - missing
- `pink_wall_banner` - missing
- `purple_wall_banner` - missing
- `red_wall_banner` - missing
- `white_wall_banner` - missing
- `yellow_wall_banner` - missing

## warped

- `warped_button` - missing
- `warped_door` - missing
- `warped_fence` - missing
- `warped_fence_gate` - missing
- `warped_fungus` - missing
- `warped_hanging_sign` - missing
- `warped_hyphae` - missing
- `warped_nylium` - missing
- `warped_planks` - missing
- `warped_pressure_plate` - missing
- `warped_roots` - missing
- `warped_shelf` - missing
- `warped_sign` - missing
- `warped_slab` - missing
- `warped_stairs` - missing
- `warped_stem` - missing
- `warped_trapdoor` - missing
- `warped_wall_hanging_sign` - missing
- `warped_wall_sign` - missing
- `warped_wart_block` - missing

## water

- `water` - partial
- `water_cauldron` - missing

## waxed

- `waxed_chiseled_copper` - missing
- `waxed_copper_bars` - missing
- `waxed_copper_block` - missing
- `waxed_copper_bulb` - missing
- `waxed_copper_chain` - missing
- `waxed_copper_chest` - missing
- `waxed_copper_door` - missing
- `waxed_copper_golem_statue` - missing
- `waxed_copper_grate` - missing
- `waxed_copper_lantern` - missing
- `waxed_copper_trapdoor` - missing
- `waxed_cut_copper` - missing
- `waxed_cut_copper_slab` - missing
- `waxed_cut_copper_stairs` - missing
- `waxed_exposed_chiseled_copper` - missing
- `waxed_exposed_copper` - missing
- `waxed_exposed_copper_bars` - missing
- `waxed_exposed_copper_bulb` - missing
- `waxed_exposed_copper_chain` - missing
- `waxed_exposed_copper_chest` - missing
- `waxed_exposed_copper_door` - missing
- `waxed_exposed_copper_golem_statue` - missing
- `waxed_exposed_copper_grate` - missing
- `waxed_exposed_copper_lantern` - missing
- `waxed_exposed_copper_trapdoor` - missing
- `waxed_exposed_cut_copper` - missing
- `waxed_exposed_cut_copper_slab` - missing
- `waxed_exposed_cut_copper_stairs` - missing
- `waxed_exposed_lightning_rod` - missing
- `waxed_lightning_rod` - missing
- `waxed_oxidized_chiseled_copper` - missing
- `waxed_oxidized_copper` - missing
- `waxed_oxidized_copper_bars` - missing
- `waxed_oxidized_copper_bulb` - missing
- `waxed_oxidized_copper_chain` - missing
- `waxed_oxidized_copper_chest` - missing
- `waxed_oxidized_copper_door` - missing
- `waxed_oxidized_copper_golem_statue` - missing
- `waxed_oxidized_copper_grate` - missing
- `waxed_oxidized_copper_lantern` - missing
- `waxed_oxidized_copper_trapdoor` - missing
- `waxed_oxidized_cut_copper` - missing
- `waxed_oxidized_cut_copper_slab` - missing
- `waxed_oxidized_cut_copper_stairs` - missing
- `waxed_oxidized_lightning_rod` - missing
- `waxed_weathered_chiseled_copper` - missing
- `waxed_weathered_copper` - missing
- `waxed_weathered_copper_bars` - missing
- `waxed_weathered_copper_bulb` - missing
- `waxed_weathered_copper_chain` - missing
- `waxed_weathered_copper_chest` - missing
- `waxed_weathered_copper_door` - missing
- `waxed_weathered_copper_golem_statue` - missing
- `waxed_weathered_copper_grate` - missing
- `waxed_weathered_copper_lantern` - missing
- `waxed_weathered_copper_trapdoor` - missing
- `waxed_weathered_cut_copper` - missing
- `waxed_weathered_cut_copper_slab` - missing
- `waxed_weathered_cut_copper_stairs` - missing
- `waxed_weathered_lightning_rod` - missing

## weathered

- `weathered_chiseled_copper` - missing
- `weathered_copper` - missing
- `weathered_copper_bars` - missing
- `weathered_copper_bulb` - missing
- `weathered_copper_chain` - missing
- `weathered_copper_chest` - missing
- `weathered_copper_door` - missing
- `weathered_copper_golem_statue` - missing
- `weathered_copper_grate` - missing
- `weathered_copper_lantern` - missing
- `weathered_copper_trapdoor` - missing
- `weathered_cut_copper` - missing
- `weathered_cut_copper_slab` - missing
- `weathered_cut_copper_stairs` - missing
- `weathered_lightning_rod` - missing

## weeping

- `weeping_vines` - missing
- `weeping_vines_plant` - missing

## wet

- `wet_sponge` - cube

## wheat

- `wheat` - missing

## wildflowers

- `wildflowers` - missing

## wither

- `wither_rose` - missing
- `wither_skeleton_skull` - missing
- `wither_skeleton_wall_skull` - missing

## wool

- `black_wool` - cube
- `blue_wool` - cube
- `brown_wool` - cube
- `cyan_wool` - cube
- `gray_wool` - cube
- `green_wool` - cube
- `light_blue_wool` - cube
- `light_gray_wool` - cube
- `lime_wool` - cube
- `magenta_wool` - cube
- `orange_wool` - cube
- `pink_wool` - cube
- `purple_wool` - cube
- `red_wool` - cube
- `white_wool` - cube
- `yellow_wool` - cube

## wool_slab

- `black_wool_slab` - missing
- `blue_wool_slab` - missing
- `brown_wool_slab` - missing
- `cyan_wool_slab` - missing
- `gray_wool_slab` - missing
- `green_wool_slab` - missing
- `light_blue_wool_slab` - missing
- `light_gray_wool_slab` - missing
- `lime_wool_slab` - missing
- `magenta_wool_slab` - missing
- `orange_wool_slab` - missing
- `pink_wool_slab` - missing
- `purple_wool_slab` - missing
- `red_wool_slab` - missing
- `white_wool_slab` - missing
- `yellow_wool_slab` - missing

## wool_stairs

- `black_wool_stairs` - missing
- `blue_wool_stairs` - missing
- `brown_wool_stairs` - missing
- `cyan_wool_stairs` - missing
- `gray_wool_stairs` - missing
- `green_wool_stairs` - missing
- `light_blue_wool_stairs` - missing
- `light_gray_wool_stairs` - missing
- `lime_wool_stairs` - missing
- `magenta_wool_stairs` - missing
- `orange_wool_stairs` - missing
- `pink_wool_stairs` - missing
- `purple_wool_stairs` - missing
- `red_wool_stairs` - missing
- `white_wool_stairs` - missing
- `yellow_wool_stairs` - missing

## zombie

- `zombie_head` - missing
- `zombie_wall_head` - missing
