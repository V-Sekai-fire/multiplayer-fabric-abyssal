#!/usr/bin/env python3
"""Build RISC-V ELF sandbox programs for multiplayer-fabric-sandbox.

Usage:
  scons                             # build all programs (auto-detects toolchain)
  scons riscv_cxx=riscv64-...      # override RISC-V C++ compiler
  scons -j4                         # parallel build

Toolchain auto-detection order (first found wins):
  1. riscv64-linux-gnu-g++-14    (Linux package: gcc-14-riscv64-linux-gnu)
  2. riscv64-unknown-linux-gnu-g++ (from riscv-gnu-toolchain build)
  3. zig c++ + ld.lld             (brew install zig lld)
  4. Error with install hints

Monorepo layout assumed:
  multiplayer-fabric/
    multiplayer-fabric-godot/       (Godot engine fork + sandbox module)
    multiplayer-fabric-sandbox/     ← this directory
    multiplayer-fabric-taskweft/    (taskweft standalone headers)
"""
import os
import shutil

# ── Directory layout ──────────────────────────────────────────────────────────
script_dir = Dir('#').abspath                         # multiplayer-fabric-sandbox/
monorepo   = os.path.dirname(script_dir)              # multiplayer-fabric/
godot_dir  = os.path.join(monorepo, 'multiplayer-fabric-godot')
api_dir    = os.path.join(godot_dir, 'modules', 'sandbox', 'program', 'cpp', 'api')
sim_dir    = os.path.join(godot_dir, 'modules', 'multiplayer_fabric_mmog')
tw_dir     = os.path.join(monorepo, 'multiplayer-fabric-taskweft', 'standalone')

# ── Toolchain auto-detection ──────────────────────────────────────────────────
_riscv_cxx_arg = ARGUMENTS.get('riscv_cxx', '')

def _detect_toolchain():
    """Return (cxx_cmd, extra_cxx_flags, extra_link_flags) for the RISC-V build."""
    if _riscv_cxx_arg:
        return _riscv_cxx_arg, [], []

    gnu_flags = ['-march=rv64gc_zba_zbb_zbs_zbc', '-mabi=lp64d']
    for cxx in ('riscv64-linux-gnu-g++-14', 'riscv64-unknown-linux-gnu-g++'):
        if shutil.which(cxx):
            print(f'SConstruct: using {cxx}')
            return cxx, gnu_flags, []

    if shutil.which('zig') and shutil.which('ld.lld'):
        lld = shutil.which('ld.lld')
        print(f'SConstruct: using zig c++ + {lld}')
        # zig c++ targets musl by default for Linux; -march not needed —
        # zig's LLVM picks rv64gc baseline for riscv64-linux-musl.
        return 'zig c++', ['-target', 'riscv64-linux-musl', '-mabi=lp64d'], [f'-fuse-ld={lld}']

    print('SConstruct: no RISC-V toolchain found.')
    print('  Option A (Linux):  apt install gcc-14-riscv64-linux-gnu')
    print('  Option B (macOS):  brew install zig lld')
    print('  Option C (any):    use build.sh (requires Docker)')
    Exit(1)

riscv_cxx, extra_cxx, extra_link = _detect_toolchain()

# ── Compiler flags ────────────────────────────────────────────────────────────
cxx_flags = [
    '-O2', '-std=gnu++23', '-DVERSION=10',
    '-fno-stack-protector', '-fno-threadsafe-statics',
] + extra_cxx

wrap_syms = [
    'memcpy', 'memset', 'memcmp', 'memmove',
    'strlen', 'strcmp', 'strncmp',
    'malloc', 'calloc', 'realloc', 'free',
]
wrap_flags = ['-Wl,' + ','.join('--wrap=' + s for s in wrap_syms)]

env = Environment(
    CXX       = riscv_cxx,
    CXXFLAGS  = cxx_flags,
    LINKFLAGS = ['-static'] + wrap_flags + extra_link + cxx_flags,
    CPPPATH   = [api_dir],
)

# ── Shared API objects (compiled once, linked into every program) ─────────────
api_srcs = Glob(os.path.join(api_dir, '*.cpp'))
api_objs = [env.Object(src) for src in api_srcs]

# ── Per-program builder ───────────────────────────────────────────────────────
def sandbox_program(name, src, extra_includes=()):
    e = env.Clone()
    if extra_includes:
        e.Append(CPPPATH=list(extra_includes))
    obj = e.Object(source=src)
    return e.Program(target=name, source=[obj] + api_objs)

# ── Programs ──────────────────────────────────────────────────────────────────
sandbox_program('jellygrid_swarm',      'jellygrid_swarm.cpp',      [sim_dir])
sandbox_program('jellygrid_power_node', 'jellygrid_power_node.cpp', [sim_dir])
sandbox_program('jellygrid_current',    'jellygrid_current.cpp',    [sim_dir])

# taskweft standalone headers live in multiplayer-fabric-taskweft/standalone/
sandbox_program('taskweft_planner', 'taskweft_planner.cpp', [tw_dir])
