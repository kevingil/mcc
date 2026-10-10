# Dedicated server

Singleplayer stays an in-process simulation. The dedicated server is a second binary that runs that same simulation without a window. The client already has a multiplayer screen. Every button on it except Cancel is disabled. `src/screen_multiplayer.c`.

The server is for people hosting on a VPS. It is not a Mojang-compatible server. An official client cannot connect, and OpenCraft does not connect to one.

## Process

`opencraft-server` links the simulation, the NBT and region code, and the socket code. It does not link raylib. `src/voxel_types.h` includes `raylib.h` today, so the server slice has to move shared types out of that include. That split is the whole slice. It does not also add sockets.

Runtime:

- Bind `0.0.0.0` and the port from `--port`, or from `PORT` when that variable is set. Default port is 25570, not 25565.
- World directory from `--world`, default `world`. The layout matches the client: `level.dat`, `region/`, and the menu index is not required.
- Tick at 20 Hz on one thread. The socket thread only queues packets.
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

Prediction and interpolation are not in the direct-connect slice. The client draws the latest Welcome and Entity positions. It will feel laggy on a distant VPS. That is acceptable until a later slice adds prediction. Do not hide that lag by letting the client decide block breaks. Place and break are server packets. The authoritative-edit slice is the one that removes the local write.

## Two players

The second-player slice is done when two clients, two processes, see each other move and see the same block change. Nametags can be the entity's name from Hello. Skins, chat, and tab list are later slices. Open one when this one is done.

## Hosting

The process is a long-lived TCP server with a disk. That rules out a free HTTP web dyno.

| Host | Fit |
| --- | --- |
| DigitalOcean droplet, or any VPS | The straightforward case. A systemd unit runs the binary and restarts it. Open TCP 25570. Put the world on the droplet disk. |
| Fly.io | A Machine plus a volume works. Publish TCP 25570. The volume is the world directory. `fly.toml` sets `PORT` only if you choose to read `PORT`. Prefer `--port 25570` in the command so it does not depend on HTTP port conventions. |
| Render | A poor fit. Free web services spin down after 15 minutes and the filesystem is wiped on restart. Web services expect HTTP. A game port wants a paid always-on instance and a persistent disk, and even then the HTTP health check is not a player connection. Prefer a VPS or Fly. If someone still deploys here, bind `0.0.0.0:$PORT`, put the world on a persistent disk, and turn spin-down off. |

Do not commit a live `fly.toml` or a Render service as part of a gameplay slice. A hosting slice can add `deploy/opencraft-server.service` and a sample `deploy/fly.toml` once the binary accepts `--port` and `--world`. Those files are examples. They do not provision an account.

Backups are a copy of the world directory after a flush. The region files are the save. There is no database in v1.

## What the client slice changes

`InitMultiplayerScreen` gains a direct-connect field: host, port, name. Join Server stays disabled until the server list slice. Realms stays a coming-soon screen. Cancel still returns to the title.

The gameplay screen, when it was entered from direct connect, sends Input and applies Chunk, Block, and Entity. Esc still pauses, and Leave disconnects. The local save path is unchanged for singleplayer.
