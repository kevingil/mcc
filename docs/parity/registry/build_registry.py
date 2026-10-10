#!/usr/bin/env python3
"""Refresh the parity registries from a misode/mcmeta summary JSON.

Status words already in the markdown are kept. Pass the registries
data.min.json from https://github.com/misode/mcmeta branch summary:

    python3 docs/parity/registry/build_registry.py /path/to/registries.min.json

The committed markdown is the tracker. This script only rebuilds it.
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
STATUS_RE = re.compile(r"^- `([a-z0-9_./]+)` - (missing|cube|partial|done)\s*$")

# Full cubes in src/voxel_types.h. Water is partial (flow), not a cube id.
CUBE_BLOCKS = {
    "acacia_leaves", "acacia_log", "acacia_planks",
    "andesite", "bedrock",
    "birch_leaves", "birch_log", "birch_planks",
    "black_concrete", "black_stained_glass", "black_terracotta", "black_wool",
    "blue_concrete", "blue_ice", "blue_stained_glass", "blue_terracotta", "blue_wool",
    "bone_block", "bookshelf", "bricks", "brown_concrete", "brown_stained_glass",
    "brown_terracotta", "brown_wool", "cactus", "chest", "chiseled_quartz_block",
    "chiseled_sandstone", "clay", "coal_block", "coal_ore", "cobblestone",
    "crafting_table", "cracked_stone_bricks", "cut_sandstone",
    "cyan_concrete", "cyan_stained_glass", "cyan_terracotta", "cyan_wool",
    "dark_oak_leaves", "dark_oak_log", "dark_oak_planks",
    "diamond_block", "diamond_ore", "diorite", "dirt",
    "emerald_block", "emerald_ore", "end_stone", "furnace",
    "glass", "glowstone", "gold_block", "gold_ore", "granite", "grass_block",
    "gravel", "gray_concrete", "gray_stained_glass", "gray_terracotta", "gray_wool",
    "green_concrete", "green_stained_glass", "green_terracotta", "green_wool",
    "hay_block", "honeycomb_block", "ice",
    "iron_block", "iron_ore", "jack_o_lantern",
    "lapis_block", "lapis_ore",
    "light_blue_concrete", "light_blue_stained_glass", "light_blue_terracotta", "light_blue_wool",
    "light_gray_concrete", "light_gray_stained_glass", "light_gray_terracotta", "light_gray_wool",
    "lime_concrete", "lime_stained_glass", "lime_terracotta", "lime_wool",
    "magenta_concrete", "magenta_stained_glass", "magenta_terracotta", "magenta_wool",
    "magma_block", "melon", "mossy_cobblestone", "mossy_stone_bricks",
    "netherrack", "oak_leaves", "oak_log", "oak_planks", "obsidian",
    "orange_concrete", "orange_stained_glass", "orange_terracotta", "orange_wool",
    "packed_ice", "pink_concrete", "pink_stained_glass", "pink_terracotta", "pink_wool",
    "prismarine", "pumpkin", "purple_concrete", "purple_stained_glass",
    "purple_terracotta", "purple_wool", "purpur_block",
    "quartz_block", "quartz_pillar",
    "red_concrete", "red_sand", "red_sandstone", "red_stained_glass",
    "red_terracotta", "red_wool", "redstone_block", "redstone_ore",
    "sand", "sandstone", "sea_lantern", "smooth_stone", "snow_block",
    "soul_sand", "sponge", "stone", "stone_bricks",
    "terracotta", "wet_sponge",
    "white_concrete", "white_stained_glass", "white_terracotta", "white_wool",
    "yellow_concrete", "yellow_stained_glass", "yellow_terracotta", "yellow_wool",
}

PARTIAL_BLOCKS = {"water"}
PARTIAL_ITEMS = {"bucket", "water_bucket"}

COLORS = (
    "light_blue", "light_gray", "magenta", "orange", "yellow", "black",
    "brown", "white", "green", "lime", "pink", "gray", "cyan", "blue",
    "purple", "red",
)
WOODS = (
    "dark_oak", "pale_oak", "mangrove", "cherry", "bamboo", "crimson",
    "warped", "acacia", "birch", "jungle", "spruce", "poplar", "oak",
)

VEHICLES_MARK = ("boat", "raft", "minecart")
PROJECTILES = {
    "arrow", "spectral_arrow", "trident", "snowball", "egg", "ender_pearl",
    "experience_bottle", "splash_potion", "lingering_potion", "fireball",
    "small_fireball", "dragon_fireball", "wither_skull", "shulker_bullet",
    "llama_spit", "wind_charge", "breeze_wind_charge", "ice_ball",
    "firework_rocket", "fishing_bobber", "eye_of_ender",
}
TECHNICAL = {
    "area_effect_cloud", "armor_stand", "block_display", "item_display",
    "text_display", "end_crystal", "experience_orb", "falling_block", "item",
    "item_frame", "glow_item_frame", "leash_knot", "lightning_bolt", "marker",
    "interaction", "painting", "tnt", "evoker_fangs", "player",
    "ominous_item_spawner", "mannequin",
}

SOURCE = (
    "IDs from the misode/mcmeta `summary` registries dump fetched 2026-10-10. "
    "That branch was on Minecraft Java Edition 26.4 Snapshot 3 "
    "(data version 5122, built 2026-10-06). "
    "Behavior target is the stable release Java Edition 26.3 "
    "(2026-09-15, protocol 777, data version 5023). "
    "A public 26.3 ID list counted 1,286 blocks, 1,658 items, and 161 entity types. "
    "This dump has a few more because it includes that snapshot."
)


def load_status(path):
    found = {}
    if not path.exists():
        return found
    for line in path.read_text().splitlines():
        match = STATUS_RE.match(line)
        if match:
            found[match.group(1)] = match.group(2)
    return found


def block_status(block_id, previous):
    if block_id in previous and previous[block_id] == "done":
        return "done"
    if block_id in PARTIAL_BLOCKS:
        return "partial"
    if block_id in CUBE_BLOCKS:
        return "cube"
    if block_id in previous and previous[block_id] in ("partial", "cube", "done"):
        return previous[block_id]
    return "missing"


def item_status(item_id, previous):
    if item_id in previous and previous[item_id] == "done":
        return "done"
    if item_id in PARTIAL_ITEMS:
        return "partial"
    if item_id in CUBE_BLOCKS:
        return "cube"
    if item_id in previous and previous[item_id] in ("partial", "cube", "done"):
        return previous[item_id]
    return "missing"


def family(entry_id):
    for wood in WOODS:
        if entry_id == wood or entry_id.startswith(wood + "_"):
            return wood
    for color in COLORS:
        if entry_id.startswith(color + "_"):
            rest = entry_id[len(color) + 1:]
            if rest:
                return rest
    return entry_id.split("_", 1)[0]


def write_grouped(path, title, ids, status_of, blurb):
    previous = load_status(path)
    groups = {}
    for entry_id in ids:
        groups.setdefault(family(entry_id), []).append(entry_id)
    counts = {"missing": 0, "cube": 0, "partial": 0, "done": 0}
    lines = [
        f"# {title}",
        "",
        blurb,
        "",
        SOURCE,
        "",
        "Edit the status word only: `missing`, `cube`, `partial`, `done`. "
        "Regenerating this file keeps a `done` mark and any `partial` or `cube` mark that is not in the default set.",
        "",
        "Status:",
        "",
        "- `missing` means OpenCraft does not simulate it.",
        "- `cube` means a full-cube stand-in exists in `src/voxel_types.h`. No states, drops, or hardness.",
        "- `partial` means some behavior exists and the slice is not done.",
        "- `done` means the owning slice's checks passed.",
        "",
    ]
    body = []
    for name in sorted(groups):
        entries = groups[name]
        body.append(f"## {name}")
        body.append("")
        for entry_id in entries:
            status = status_of(entry_id, previous)
            counts[status] += 1
            body.append(f"- `{entry_id}` - {status}")
        body.append("")
    lines.append(
        f"{len(ids)} ids. "
        f"{counts['done']} done, {counts['partial']} partial, "
        f"{counts['cube']} cube, {counts['missing']} missing."
    )
    lines.append("")
    lines.extend(body)
    path.write_text("\n".join(lines))
    return counts


def write_entities(path, ids):
    previous = load_status(path)
    buckets = {"mobs": [], "vehicles": [], "projectiles": [], "technical": []}
    for entry_id in ids:
        if any(mark in entry_id for mark in VEHICLES_MARK):
            buckets["vehicles"].append(entry_id)
        elif entry_id in PROJECTILES:
            buckets["projectiles"].append(entry_id)
        elif entry_id in TECHNICAL:
            buckets["technical"].append(entry_id)
        else:
            buckets["mobs"].append(entry_id)
    counts = {"missing": 0, "cube": 0, "partial": 0, "done": 0}
    lines = [
        "# Entities",
        "",
        "Mobs are the living creatures. Vehicles, projectiles, and technical entities are separate so a mob slice does not have to spawn minecarts.",
        "",
        SOURCE,
        "",
        "Textures under `src/resources/textures/entity/` are Good Vibes art. They are not a simulation.",
        "",
        "Edit the status word only: `missing`, `cube`, `partial`, `done`.",
        "",
    ]
    for label in ("mobs", "vehicles", "projectiles", "technical"):
        lines.append(f"## {label}")
        lines.append("")
        for entry_id in buckets[label]:
            status = previous.get(entry_id, "missing")
            if status not in counts:
                status = "missing"
            counts[status] += 1
            lines.append(f"- `{entry_id}` - {status}")
        lines.append("")
    lines.insert(8, f"{len(ids)} entity types. {len(buckets['mobs'])} mobs, {len(buckets['vehicles'])} vehicles, {len(buckets['projectiles'])} projectiles, {len(buckets['technical'])} technical. {counts['done']} done, {counts['missing']} missing.")
    lines.insert(9, "")
    path.write_text("\n".join(lines))
    return counts, {key: len(value) for key, value in buckets.items()}


def write_flat(path, title, ids, blurb):
    previous = load_status(path)
    counts = {"missing": 0, "cube": 0, "partial": 0, "done": 0}
    lines = [
        f"# {title}",
        "",
        blurb,
        "",
        SOURCE,
        "",
        "Edit the status word only: `missing`, `cube`, `partial`, `done`.",
        "",
    ]
    body = []
    for entry_id in ids:
        status = previous.get(entry_id, "missing")
        if status not in counts:
            status = "missing"
        counts[status] += 1
        body.append(f"- `{entry_id}` - {status}")
    lines.append(f"{len(ids)} ids. {counts['done']} done, {counts['missing']} missing.")
    lines.append("")
    lines.extend(body)
    lines.append("")
    path.write_text("\n".join(lines))
    return counts


def write_catalog(path, registries):
    keys = (
        "attribute", "fluid", "dimension", "dimension_type", "enchantment",
        "mob_effect", "game_rule", "worldgen/structure",
    )
    lines = [
        "# Other registries",
        "",
        "These are not blocks or items. Slices refer to them by id. Status starts at missing.",
        "",
        SOURCE,
        "",
    ]
    for key in keys:
        values = registries[key]
        lines.append(f"## {key}")
        lines.append("")
        lines.append(f"{len(values)} ids.")
        lines.append("")
        for entry_id in values:
            lines.append(f"- `{entry_id}`")
        lines.append("")
    path.write_text("\n".join(lines))


def main():
    if len(sys.argv) != 2:
        print("usage: build_registry.py registries.min.json", file=sys.stderr)
        return 1
    registries = json.loads(Path(sys.argv[1]).read_text())
    missing = sorted(CUBE_BLOCKS - set(registries["block"]))
    if missing:
        print("cube ids not in the block registry:", ", ".join(missing), file=sys.stderr)
        return 1
    block_counts = write_grouped(
        ROOT / "blocks.md",
        "Blocks",
        registries["block"],
        block_status,
        "One line per Java Edition block id. Block states (facing, waterlogged, age) are not separate ids. Stairs and doors show up once, and the state work is a slice of its own.",
    )
    item_counts = write_grouped(
        ROOT / "items.md",
        "Items",
        registries["item"],
        item_status,
        "One line per Java Edition item id. A `cube` item is only the creative block entry in the current inventory. The bucket items are `partial` because the hotbar can pick up and place a water source.",
    )
    entity_counts, entity_kinds = write_entities(ROOT / "entities.md", registries["entity_type"])
    biome_counts = write_flat(
        ROOT / "biomes.md",
        "Biomes",
        registries["worldgen/biome"],
        "OpenCraft generates one noise field with a shore, trees, and a water level. It has no biome id.",
    )
    write_catalog(ROOT / "catalogs.md", registries)
    summary = [
        "# Registry counts",
        "",
        SOURCE,
        "",
        "| Registry | Count | OpenCraft |",
        "| --- | ---: | --- |",
        f"| Blocks | {len(registries['block'])} | {block_counts['done']} done, {block_counts['partial']} partial, {block_counts['cube']} cube, {block_counts['missing']} missing |",
        f"| Items | {len(registries['item'])} | {item_counts['done']} done, {item_counts['partial']} partial, {item_counts['cube']} cube, {item_counts['missing']} missing |",
        f"| Entity types | {len(registries['entity_type'])} | {entity_kinds['mobs']} mobs, {entity_kinds['vehicles']} vehicles, {entity_kinds['projectiles']} projectiles, {entity_kinds['technical']} technical, all missing |",
        f"| Biomes | {len(registries['worldgen/biome'])} | {biome_counts['missing']} missing |",
        f"| Enchantments | {len(registries['enchantment'])} | missing |",
        f"| Mob effects | {len(registries['mob_effect'])} | missing |",
        f"| Attributes | {len(registries['attribute'])} | movement is hardcoded in `src/player.c`, not an attribute |",
        f"| Fluids | {len(registries['fluid'])} | water partial, lava missing |",
        f"| Dimensions | {len(registries['dimension'])} | overworld only, and only y 0..127 |",
        f"| Worldgen structures | {len(registries['worldgen/structure'])} | missing |",
        f"| Game rules | {len(registries['game_rule'])} | missing |",
        "",
    ]
    (ROOT / "SUMMARY.md").write_text("\n".join(summary))
    print("blocks", block_counts)
    print("items", item_counts)
    print("entities", entity_counts, entity_kinds)
    print("biomes", biome_counts)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
