Code both binaries link. It must not include `raylib.h`.

`nbt`, `anvil`, `chunk_codec`, and `level_data` live here. World, fluids, and the player stay in `client/` until `voxel_types.h` no longer includes raylib.

`protocol`, `wire`, `sha256`, and `db` are the multiplayer session. `db` opens `opencraft.db` in WAL mode and hashes passwords. sqlite-vec is optional. The server calls SQLite on its own thread until the S31 completion API exists.
