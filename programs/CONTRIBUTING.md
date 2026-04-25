# Contributing

A self-contained C++ simulation library and test harness for
multiplayer-fabric computation kernels — jellygrid swarm dynamics,
power-node flow, and the Taskweft HTN planner.  The source is RISC-V
compatible and is consumed by the `godot-sandbox` module inside the
`multiplayer-fabric-godot` fork, which runs these kernels inside a
sandboxed Godot scripting environment.  A cross-compilation toolchain
is provided via `SConstruct` (auto-detects toolchain) or Docker via `build.sh`.

Built strictly red-green-refactor: every feature is driven by a failing
test, committed when green, then any cleanup is done with the test
still green.  ASAN + UBSAN run on every test.

## Guiding principles

- **RED first, always.** Before writing implementation code, write a
  test that compiles and fails at runtime.  Mutation-test it by briefly
  breaking the code to prove the assertions are load-bearing.
- **ASAN is part of the contract.** `-fsanitize=address,undefined`
  runs on every `cmake --build` in Debug mode.  An ASAN or UBSAN
  finding is a RED, not a warning.
- **No dynamic allocation in hot paths.** Jellygrid and planner kernels
  must operate on stack or pre-allocated arena memory.  `new` / `delete`
  in inner loops are forbidden; use `std::array`, `std::span`, or a
  bump allocator.
- **RISC-V portability.** Do not use x86/ARM intrinsics without a
  fallback path.  All SIMD optimizations must be guarded by a
  compile-time feature flag and have a scalar reference implementation
  that is tested independently.
- **Commit every green.** One commit per cycle.  Messages use sentence
  case, e.g. `Add jellygrid power node boundary condition`.  Do not use
  Conventional Commits prefixes (`feat:`, `fix:`, `chore:`, etc.).

## Workflow

### Build RISC-V ELFs (primary)

`SConstruct` auto-detects the toolchain — run `scons` from this directory:

```sh
scons        # detects riscv64-linux-gnu-g++, zig+lld, or prints install hints
scons -j4    # parallel
```

Install the toolchain (pick one):

```sh
# macOS — zig + lld (no extra tap required)
brew install zig lld

# Linux (Debian/Ubuntu)
apt install gcc-14-riscv64-linux-gnu

# Any platform — Docker fallback (no local install needed)
./build.sh
```

### Development builds with sanitizers

Use the system C++ compiler with ASAN + UBSAN to test host-architecture logic:

```sh
c++ -std=c++23 -fsanitize=address,undefined -g \
    -I modules/multiplayer_fabric_mmog \
    jellygrid_swarm_sim.hpp  # header-only, compile via any translation unit
```

The simulation headers (`jellygrid_swarm_sim.hpp`, etc.) have no Godot or
api.hpp dependency and can be unit-tested directly on the host.

## Design notes

### Jellygrid swarm model

Each node in the jellygrid is a mass-spring damper connected to its
neighbours via a shared `current` value.  `jellygrid_current.cpp`
implements the update rule; `jellygrid_swarm.cpp` owns the topology.
Power-node injection (`jellygrid_power_node.cpp`) sets boundary
conditions.  The integration step is explicit Euler; the timestep is
fixed at compile time via `JELLYGRID_DT`.  Do not make the timestep
a runtime parameter — it must be a compile-time constant for RISC-V
pipeline predictability.

### Taskweft planner kernel

`taskweft_planner.cpp` is a standalone C++ port of the Elixir HTN
planner.  It shares no code with the Elixir library; it is a reference
implementation for embedded/RISC-V deployment.  Keep the two
implementations in sync by running both against the same task network
fixture and asserting identical plan output.

### godot-sandbox integration

The kernels in this library are loaded by the `godot-sandbox` Godot
module in `multiplayer-fabric-godot`.  The sandbox compiles the source
to RISC-V bytecode and executes it in a sandboxed VM inside the engine.
Changes to public API signatures (function names, argument order, return
types) require a matching update in the Godot module's binding layer.
Validate integration by building the Godot fork with the updated sandbox
binary and running the in-editor sandbox test scene.

### RISC-V toolchain

`SConstruct` auto-detects in priority order:
1. `riscv64-linux-gnu-g++-14` — Linux package `gcc-14-riscv64-linux-gnu`
2. `riscv64-unknown-linux-gnu-g++` — from `riscv-gnu-toolchain` source build
3. `zig c++` + `ld.lld` — `brew install zig lld` on macOS (uses musl libc)
4. `./build.sh` — Docker fallback, no local toolchain required

