# Building OpenCraft with agents

The parity list is larger than one context window. The way through it is one slice per session, with a check that session can run. This file is the rule. `SLICES.md` is the queue.

The shape comes from two places. Cursor's agent guide (Lee Robinson, 9 January 2026, [Best practices for coding with agents](https://cursor.com/blog/agent-best-practices)) is the one that matches this toolchain: plan before editing, start a fresh session when the task changes, keep always-on instructions short, and give the agent a result it can check. The other is the tracer-bullet split used for big features: a slice is a narrow path through every layer it needs, sized to one session, with the slices that block it named. A layer called "networking" or "all the blocks" is not a slice.

## What a session is allowed to do

1. Read `docs/parity/README.md`, this file, and the one slice in `SLICES.md`.
2. If that slice's status is `DOING` or `DONE`, stop. If it is `BLOCKED`, stop. If an earlier slice is still `TODO` or `DOING`, stop. Take the earliest slice whose blockers are all `DONE`.
3. Set the status to `DOING` in the first commit, and name the slice in the commit message.
4. Implement only that slice. The `Owns` lines are the files you may create or edit. A file outside `Owns` is a sign the slice was drawn wrong. Stop and say so, rather than editing the file.
5. Run the slice's check. `bash .agents/build.sh` when the slice compiles the game. `bash .agents/test.sh` when the slice changes what the window draws. A headless test named in the slice is the real check for simulation work. The GUI smoke test does not prove walk speed.
6. Set the status to `DONE` only after the check passes. Update the registry lines and the `SYSTEMS.md` row the slice names. Leave every other slice alone.
7. One branch, one PR. Do not open a follow-up branch in the same session for the next slice.

If the check fails and the approach is wrong, revert to the plan in the slice. Do not stack patches on a tick model the test already rejected.

## How a slice is written

Each slice in `SLICES.md` has a status, the slices that block it, the files it owns, a done-when list, and an out-of-scope list. Done-when is something a later agent can run or see. "Add physics" is not a slice. "Walk speed on flat stone is 4.317 blocks/s in `tests/movement_test`" is.

Shared contracts live in the docs, not in a slice's head:

- Tick, speeds, and hitboxes: `PHYSICS.md`
- Height, light, dimensions: `WORLD.md`
- Packets and ports: `SERVER.md`
- Shader order and the ray-tracing ban: `SHADERS.md`
- Ids: `registry/`

A slice that needs a new constant reads it from those files. If the constant is missing, the slice says so and stops. It does not publish a new gravity.

C style is `CONVENTIONS.md`. Four spaces, braces on their own lines, raylib naming for functions that already follow it. New simulation files use the same style. Do not add a C++ class hierarchy beside the C code. Do not add a scripting language.

New tests follow `tests/save_format_test.c`: a small C binary in `CMakeLists.txt`, assertions, no window. Wire the binary into the slice's done-when command.

## What not to parallelize

S01 through S13 share the simulation and the save format. They are serial. Two sessions on two of them will conflict, and the second merge will silently drop a tick change.

After S17, a content slice may run beside another content slice only when both `Owns` lists are disjoint and neither status is `DOING` on a shared file. Block families still collide if they edit the same table. Prefer one content slice at a time until family files are actually split. `SLICES.md` does not promise parallelism it has not separated.

S30 stays `BLOCKED`. Parallel sessions do not make it available.

Wide refactors are not slices. The server's removal of `raylib.h` from shared headers is S12, and it is the one wide change we already accepted. Do it by moving types, then compiling the old client against the new header, then running the smoke test. Do not rewrite the mesher in that PR.

## Adding a slice

The queue stops at S30 on purpose. A mob, a biome, or a block family beyond those slices is a new slice, not a drive-by in an existing one.

Copy this into `SLICES.md` above S28, pick the next free id, and set `Blocked by` to the slice that created the seam you need. S23 is the seam for a passive mob. S09 is the seam for a block with state. S04 is the seam for an item.

```
### S31: Pig saddle
Status: TODO
Blocked by: S23
Owns: src/sim/pig.c, tests/pig_test.c
Done when:
- [ ] The check in the slice passes
Out of scope:
- Riding any other mob
```

Then implement it in a later session. The session that only adds the slice does not also write the code, unless the maintainer asked for both.

Registry status words are `missing`, `cube`, `partial`, and `done`. `docs/parity/registry/build_registry.py` rebuilds the lists from a misode summary JSON and keeps a `done` mark. Day to day, edit the line. Do not hand-reorder the generated lists.

## Prompt

Paste this as the task, with the slice id filled in:

```
Read docs/parity/AGENT.md, docs/parity/README.md, and slice Sxx in docs/parity/SLICES.md.
Read the contract docs that slice points at.
Implement that slice only. Set it to DOING in the first commit.
Follow CONVENTIONS.md. Do not copy Mojang assets or source.
Run the slice's check. Set it to DONE only if the check passes, and update the registry rows the slice names.
Do not start another slice. Do not start S30.
```

## Review

Read the diff against the slice, not against "closer to Minecraft." Extra ids, a second protocol, a shader in a physics PR, and a ray tracer are defects even when they run. The GUI smoke test is required when the slice changes drawing or menus. A movement change needs the headless test as well.
