# HUD, inventory, colors, biomes, and house tour

Status: Original scope is done. The HUD and inventory screens are in the game and match the vanilla layout. Everything still open below is scope added after that.
Branch: `kevin/cursor/ui-parity-biomes-b258`. PR base: `kevin/cursor/multiplayer-session-b258`.

## What you asked for

1. Rebuild the HUD and the inventory picker as near-exact copies of vanilla. Follow `CONVENTIONS.md`. No Mojang textures, sounds, or source. Keep the Good Vibes (Acaitart, CC-BY) credit.
2. Audit and improve or rebuild biome generation. Somewhat more realistic than vanilla, nothing extreme, similar proportions.
3. Build a house in every biome, tour inside each one, and record a video of all of it.
4. Review what is left for full parity.
5. Color-match water and basic blocks (grass, sand) to vanilla.
6. Clear text contrast everywhere. The F3 text needs vanilla's light semi-transparent band behind each line.

## Where things stand right now

Original request: rebuild the HUD and the inventory picker. That is done. Code is committed and pushed (`3153394` through `876544f`). `bash .agents/build.sh` and `bash .agents/test.sh` pass. Screenshots are in `/opt/cursor/artifacts`.

Added later, and still open: color matching, F3 text contrast, biome generation, a house in every biome with a tour video, and a parity write-up.

## Checklist

### Original scope: HUD and inventory (done)
- [x] GUI core: scaling, sprites painted in code, font and shadow, item icons, tooltips (`client/gui.c`)
- [x] HUD drawing: hotbar, selector, offhand, inverted crosshair, attack indicator, hearts, food, air, XP bar, held-item name, chat, action bar (`client/hud.c`)
- [x] Inventory player model (`client/player_model.c`) and text field (`client/text_field.c`)
- [x] Crafting recipes and vanilla matching (`client/crafting.c`)
- [x] Inventory, crafting table, and creative screens rewritten (`client/inventory_ui.c`) and compiled
- [x] Wire `client/screen_gameplay.c`: `HudUpdate`, `HudDraw`, `HudDrawMessages`. F3 only toggles when no screen is open. F3+F4 switches Survival/Creative and prints "Set own game mode to ...". Removed "Click to play" and the world name and seed at the top.
- [x] `bash .agents/build.sh` passes and `bash .agents/test.sh` passes
- [x] Commit in small logical commits, push, update the PR description with screenshots
- [x] Survival HUD and survival inventory screenshotted and compared by eye to `/tmp/ref`. Layout matches. Empty armor icons corrected to `#555555`.

Small leftovers, not part of the open plan: F1 to hide the HUD, a hand pass over click rules, and saving game mode, stack counts, and the offhand. The recipe book stays out and belongs in the parity write-up.

### Added scope, still open

### 1. Text contrast
- [x] F3 screen in the game font: each line on a 0x90505050 band, text 0xE0E0E0, left and right columns like vanilla. FPS chart only with F3+Alt.
- [x] Chat already had a half-opaque black band. Action bar and the held-item name use a solid black shadow. Tooltips already had a dark background.

### 2. Color matching
Measured Good Vibes averages that are far from vanilla: sand `#FED859` (saturated yellow), water `#2FEBD8` (cyan), grass top `#6FAC44`, stone `#9B9A83` (warm), oak leaves `#129201`.
- [x] Correct those textures at load time in `client/voxel_renderer.c` (hue and saturation shift toward the target, keeping the pack's pixel detail). The PNG files and the credit stay untouched.
- [x] Water gets a blue shift toward `#3F76E4` and alpha 170
- [x] World, hotbar, and inventory share the atlas, so they show the same color for each block

### 3. Biome generation
Audit result: `BiomeAt` in `client/biomes.c` used to pick from an alphabetical list of 68 biomes with one value-noise field.
- [x] Overworld placement uses temperature, humidity, continentalness, erosion, and weirdness. Neighbors stay in the same climate family.
- [x] A sample of columns was about one third ocean. Peaks stay rare.
- [x] Height comes from continentalness and erosion. A snow line follows temperature.
- [x] Rivers carve toward the waterline. Steep coasts stay stone. Tour plateaus are unchanged.
- [x] Nether, End, and cave biomes leave the overworld. They stay on the tour.
- [x] The 68-biome tour still walks a strip per biome.

### 4. House in every biome, tour, video
- [x] `MCC_BIOME_TOUR` builds one small house per strip: local wood or stone, a door opening, glass windows, a roof, and inside a crafting table, furnace, chest, bookshelf, and glowstone
- [x] The camera starts outside, walks through the door, and looks around inside (about 4 seconds per biome)
- [x] A tour video is in `/opt/cursor/artifacts`. The HUD and inventory demo from the original scope is already there.

### 5. Parity review
- [x] `docs/parity/REMAINING.md` lists what is still missing for full parity

### 6. Delivery of the added scope
- [x] HUD and inventory artifacts are in `/opt/cursor/artifacts`, and those commits are pushed
- [x] Added-scope screenshots and the tour video are in `/opt/cursor/artifacts`

## Decisions I need from you

1. **Colors.** Slice S32 in `docs/parity/SLICES.md` says Good Vibes textures with baked-in color stay untinted. You asked for vanilla colors. This plan follows you and color-corrects at load time. OK?
2. **Recipe book.** I plan to leave it out and list it in the parity review, so the time goes to biomes and the tour. OK?
3. **Dropping items.** There are no item entities, so in survival a click outside the inventory window keeps the stack in your hand instead of destroying it. Creative deletes it. OK?
