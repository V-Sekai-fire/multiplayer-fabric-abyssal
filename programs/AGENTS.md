# AGENTS.md — multiplayer-fabric-sandbox

Guidance for AI coding agents working in this submodule.

## What this is

C++23 RISC-V sandbox programs compiled for the `godot-sandbox` module inside
`multiplayer-fabric-godot`. Produces RISC-V ELF kernels that run inside the
sandboxed Godot extension. Changing a public kernel symbol requires updating
the binding table in the godot module at the same time.

## Build

`SConstruct` is the primary build system. It auto-detects the RISC-V toolchain:

```sh
scons          # auto-detect toolchain, build all ELFs
scons -j4      # parallel build
```

Toolchain detection order:

| Priority | Toolchain | Install |
|----------|-----------|---------|
| 1 | `riscv64-linux-gnu-g++-14` | Linux: `apt install gcc-14-riscv64-linux-gnu` |
| 2 | `riscv64-unknown-linux-gnu-g++` | from `riscv-gnu-toolchain` source build |
| 3 | `zig c++` + `ld.lld` | macOS: `brew install zig lld` |
| fallback | Docker | `./build.sh` (requires Docker, no toolchain install needed) |

Override the compiler explicitly:

```sh
scons riscv_cxx=riscv64-linux-gnu-g++-14
```

For ASAN/UBSAN testing of simulation logic, compile the header-only sim files directly with the host compiler (see CONTRIBUTING.md).

## Key files

| Path | Purpose |
|------|---------|
| `SConstruct` | Primary SCons build — auto-detects RISC-V toolchain |
| `build.sh` | Docker fallback — builds ELFs using a container with the cross-compiler |
| `jellygrid_swarm.cpp` | Swarm simulation kernel |
| `jellygrid_power_node.cpp` | Power-node simulation kernel |
| `jellygrid_current.cpp` | Current-flow simulation kernel |
| `taskweft_planner.cpp` | HTN planner + HRR memory kernel (taskweft standalone headers) |

## Dependencies

- `godot-sandbox` api headers from the godot fork's
  `modules/sandbox/program/cpp/api/`
- `multiplayer_fabric_mmog` sim headers (pure C++, no Godot or api.hpp)
- `multiplayer-fabric-taskweft/standalone/` headers (header-only C++)

## Conventions

- All kernels must be RISC-V compatible — no host-only intrinsics.
- Test integration by loading the ELF into the godot fork's Sandbox class.
- Commit message style: sentence case, no `type(scope):` prefix.
  Example: `Add power-node current propagation to jellygrid kernel`
