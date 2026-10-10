# Dedicated server

The tree is three directories, not two.

| Directory | What links it | Raylib |
| --- | --- | --- |
| `client/` | the game binary `mcc` | yes |
| `common/` | `mcc` and `opencraft-server` | no |
| `server/` | `opencraft-server` only | no |

`client/` is the raylib program: screens, the renderer, input, world, fluids, and `resources/`. Launch it from `client/` so those relative paths resolve. `common/` is the static library both binaries link. It holds NBT, the region files, the chunk codec, and `level.dat`. World and player code stay in `client/` until `voxel_types.h` stops including `raylib.h`. The server does not include the client, and the client does not include `server/main.c`. Protocol code lands in `net/` in S13, and both binaries link that. It is not a fourth copy of the world.

`server/main.c` builds today and exits. It does not open a socket and it does not tick a world. S12 replaces that stub.

Singleplayer stays an in-process simulation. The dedicated server is a second binary that runs that same simulation without a window. The client already has a multiplayer screen. Every button on it except Cancel is disabled. `client/screen_multiplayer.c`.

The server is for people hosting on a VPS. It is not a Mojang-compatible server. An official client cannot connect, and OpenCraft does not connect to one.

## Process

`opencraft-server` links the simulation, the NBT and region code, and the socket code. It does not link raylib. `client/voxel_types.h` includes `raylib.h` today, so the server slice has to move shared types out of that include. That split is the whole slice. It does not also add sockets.

Runtime:

- Bind `0.0.0.0` and the port from `--port`, or from `PORT` when that variable is set. Default port is 25570, not 25565.
- World directory from `--world`, default `world`. The layout matches the client: `level.dat`, `region/`, and the menu index is not required.
- Tick at 20 Hz on the sim thread. The socket thread only queues packets. See `PHYSICS.md` for the partial tick and the stored subtick.
- Log to stdout. A VPS supervisor collects it.
- Stop on SIGTERM after flushing the world.

The first binary loads a world, ticks, and exits. No network yet. The protocol slice adds the socket.

## Protocol version 1

Little-endian. A frame is `u32 payload_length` followed by that many bytes. The payload starts with `u16 packet_id`.

Protocol number is 1. A mismatch closes the connection. Names are UTF-8, at most 16 bytes.

Client to server:

| id | name | body |
| --- | --- | --- |
| 1 | Hello | `u16 protocol`, `u8 name_len`, name |
| 2 | Input | `u32 tick`, `i16 mouse_dx`, `i16 mouse_dy`, `u16 buttons` |

Button bits, from bit 0: forward, back, left, right, jump, sprint, sneak, attack, use, slot-next, slot-prev. The server simulates the player. The client does not send a position in v1.

Server to client:

| id | name | body |
| --- | --- | --- |
| 1 | Welcome | `u16 protocol`, `u32 entity_id`, `i32 seed`, `u8 dimension`, `f32 x y z yaw pitch` |
| 2 | Chunk | `i32 chunk_x`, `i32 chunk_z`, then sections. Each section: `i8 section_y`, `u16 palette_len`, palette of string ids (`u8 len`, bytes), then 4096 palette indexes or one id if the section is uniform |
| 3 | Block | `i32 x y z`, `u8 id_len`, string id, `u16 state` |
| 4 | Entity | `u32 entity_id`, `u8 kind` (0 remove, 1 move), `f32 x y z yaw pitch` |
| 5 | Reject | `u8 reason_len`, text |

Dimension byte: 0 overworld, 1 nether, 2 end. Until those dimensions exist, the server sends 0.

Chunk packets use the section layout from `WORLD.md`. The protocol slice is blocked by the height slice so the packet is not invented twice. String ids are the registry ids. Unknown ids are air.

v1 has no chat, no encryption, and no compression. A shared secret can wait. Operators who need a private server put it behind a firewall or a tunnel.

### Session packets

The playable session uses the frame above and adds account packets. Chunk section packets are not sent yet. The shared map is the world seed plus every block edit. Clients generate terrain from the seed and apply the edits. Block ids are the client's `BlockType` values (`u16`), not registry strings, because that string table is not in the binary yet.

Client to server, after Hello (`u16 protocol`):

| id | name | body |
| --- | --- | --- |
| 3 | Register | `u8` username, `u8` password, `u8` email. Email may be empty. |
| 4 | Login | `u8` username, `u8` password |
| 5 | Join | empty |
| 6 | Block | `i32 x y z`, `u16 block` |
| 7 | Inventory | `u8 selected slot`, 9 hotbar stacks (`u16 block`, `u16 count`), 45 inventory stacks in the same shape |
| 8 | Disconnect | empty |

Server to client:

| id | name | body |
| --- | --- | --- |
| 1 | Welcome | as in the table above. Sent on Join, not on Hello. |
| 3 | Block | `i32 x y z`, `u16 block`. Broadcast to every joined client. |
| 5 | Reject | `u8 code`, `u8` text |
| 6 | Inventory | same body as the client inventory packet |
| 7 | Snapshot | `u32 count`, then that many `i32 x y z`, `u16 block` |
| 8 | LoginOk | `u32 account id`, `u8` name |
| 9 | RegisterOk | same body as LoginOk |

A bad password, a taken username, a bad email, and a dead server are reported with Reject. The client shows that text. Passwords are SHA-256 of a random salt plus the password. The database stores the hash and the salt, never the password.

SQLite `sqlite3_step` runs on the server thread. There is no Kore worker in this slice, so a completion queue would only move the same write onto another thread. S31 still owns that API. `extension_status` records whether sqlite-vec loaded. The feature does not require it.

Prediction and interpolation are not in the direct-connect slice. The client draws the latest Welcome and Entity positions. It will feel laggy on a distant VPS. That is acceptable until a later slice adds prediction. Do not hide that lag by letting the client decide block breaks. Place and break are server packets. The authoritative-edit slice is the one that removes the local write.

## Async I/O and the database

One process. Two listeners. The language stays C.

The simulation is a function in `common/`: `Tick` takes the world and the inputs and returns the next world. The client calls it from the raylib frame. The server calls it from the sim thread at 20 Hz. That function is synchronous. Chunks may later be stepped in parallel, and the tick does not finish until those jobs join. Callbacks inside entity movement would drop the function the client shares.

Async is everything around that function. HTTP handlers, outbound requests, and database calls do not run inside `Tick`, and they do not block a Kore worker. High-performance code in the web routes means the worker submits work and resumes on completion.

Kore (ISC) is the HTTP server. It owns routes, the request lifecycle, and the async outbound HTTP client through its libcurl integration. It does not own the simulation. The sim thread starts before Kore's loop and keeps running while Kore serves HTTP.

SQLite cannot make `sqlite3_step` nonblocking. The official API reads and writes on the calling thread. Wrappers that return a future still run `sqlite3_step` on a worker. A true async SQLite, with the bytecode waiting on I/O, does not exist in the library we link. We do not invent one. The caller-facing API is still asynchronous: `common/db` takes a query and a completion. The SQLite backend runs `sqlite3_step` on one writer thread and posts the completion back to Kore. WAL mode lets a reader connection proceed beside the writer. The HTTP worker never calls `sqlite3_step` itself.

That completion API is the seam for a later Postgres backend. Routes and `Tick` keep calling `common/db`. A Postgres implementation can use Kore's async driver behind the same completion. `opencraft.db` and sqlite-vec stay the storage until that backend exists. Switching storage is a new slice, not a flag sprinkled through the routes.

| Piece | Choice |
| --- | --- |
| Process | `opencraft-server` only. The sim, game packets, and HTTP share it. |
| Game port | TCP 25570. Players, chunks, blocks. |
| HTTP port | TCP 8080. Health and JSON routes. Kore. |
| Database | One SQLite file, `opencraft.db`, in the world directory. Accounts, server rows, and later feature tables go here. |
| Vectors | Load [sqlite-vec](https://github.com/asg017/sqlite-vec) when the file is opened. No vector table until a feature stores embeddings. |
| SQL access | `common/db`. Same types and the same completion API for the client and the server. |
| SQLite thread | One writer. Callers submit and complete. `Tick` and Kore workers never call `sqlite3_step`. |
| Later Postgres | A second backend behind `common/db`. Not the current storage. |
| Passwords | A real password hash when account rows land. No plaintext column. |

`common/` holds the row types, the query functions, and the completion API. It does not include Kore, libcurl, or raylib. `server/` holds the Kore routes and the queues. Singleplayer calls the same `common/db` API. The SQLite backend may run the completion inline when there is no HTTP loop.

A second process is the right split only when several world servers must share one `opencraft.db`. That is not this deploy. One VPS runs one binary and one world directory.

## Two players

The second-player slice is done when two clients, two processes, see each other move and see the same block change. Nametags can be the entity's name from Hello. Skins, chat, and tab list are later slices. Open one when this one is done.

## Hosting

The process is a long-lived TCP server with a disk. That rules out a free HTTP web dyno.

| Host | Fit |
| --- | --- |
| DigitalOcean droplet, or any VPS | The straightforward case. A systemd unit runs the binary and restarts it. Open TCP 25570 and TCP 8080. Put the world and `opencraft.db` on the droplet disk. |
| Fly.io | A Machine plus a volume works. Publish TCP 25570 and TCP 8080. The volume is the world directory. `fly.toml` sets `PORT` only if you choose to read `PORT`. Prefer `--port 25570` and `--http-port 8080` so the game port does not follow an HTTP convention. |
| Render | A poor fit. Free web services spin down after 15 minutes and the filesystem is wiped on restart. Web services expect HTTP. A game port wants a paid always-on instance and a persistent disk, and even then the HTTP health check is not a player connection. Prefer a VPS or Fly. If someone still deploys here, bind `0.0.0.0:$PORT`, put the world on a persistent disk, and turn spin-down off. |

Do not commit a live `fly.toml` or a Render service as part of a gameplay slice. A hosting slice can add `deploy/opencraft-server.service` and a sample `deploy/fly.toml` once the binary accepts `--port` and `--world`. Those files are examples. They do not provision an account.

Backups are a copy of the world directory after a flush. The region files are the world save. `opencraft.db` sits next to them and is part of that copy.

## What the client slice changes

`InitMultiplayerScreen` gains a direct-connect field: host, port, name. Join Server stays disabled until the server list slice. Realms stays a coming-soon screen. Cancel still returns to the title.

The gameplay screen, when it was entered from direct connect, sends Input and applies Chunk, Block, and Entity. Esc still pauses, and Leave disconnects. The local save path is unchanged for singleplayer.
