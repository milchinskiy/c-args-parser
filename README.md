# c-args-parser

**Header‑only, zero‑alloc, callback-based command‑line parser for C (C11).**
Subcommands (like `git`), option groups, env‑var defaults, positional schemas,
pretty help, size parsers, auto‑generated docs (Markdown & man), and shell
completions (bash/zsh/fish). No dependencies. Thread‑safe (no globals).

> Header file: `c-args-parser.h` (install path `include/c-args-parser/c-args-parser.h`).

---

## Highlights

- **Header‑only**: just add one file to your project.
- **Zero allocations** in the parser (no `malloc` inside the library). You control memory.
- **Callback design**: every option invokes your function (`(value, user)`).
- **Subcommands** with aliases and their own flags (e.g., `remote add`, `remote remove`).
- **Mutually exclusive groups**
  - `XOR` → at most one flag in the group.
  - `REQ_ONE` → exactly one flag required (env/defaults count).
- **Env‑var defaults** and string defaults (applied _before_ argv; CLI overrides).
- **Positional schemas**: declare `min..max` occurrences for each positional item.
- **Pretty help**: width‑aware wrapping, colors (respects `NO_COLOR`), auto `--help/-h`.
- **Built‑ins** (optional): `--version/-v`, `--author` at the root level.
- **Docs**: emit **Markdown** and **man(7)** from your command tree.
- **Completions**: generate shell completion for **bash**, **zsh**, **fish**.
- **Typed helpers**: `read_int`, `read_size_{si,iec}`, `fmt_bytes` (SI/IEC aware).
- **Thread‑safe**: no global mutable state; pass your `user` pointer through.
- **No deps**.

---

## File layout (repo snapshot)

```
src/c-args-parser.h         # the library (header-only)
examples/                   # small, real-world examples
  01_hello.c
  02_cp_like.c
  03_remote.c
  04_groups.c
  05_docs_completion.c
  06_env_defaults.c
  07_sizes.c
tests/cargs.tests.c         # unit-style tests (ASan/UBSan friendly)
meson.build                 # builds library, tests, examples
```

### Examples (browse the code)

- Hello + flags: [`examples/01_hello.c`](examples/01_hello.c)
- cp‑like positionals: [`examples/02_cp_like.c`](examples/02_cp_like.c)
- git‑like subcommands + aliases: [`examples/03_remote.c`](examples/03_remote.c)
- Mutually‑exclusive groups (`XOR`, `REQ_ONE`): [`examples/04_groups.c`](examples/04_groups.c)
- Docs & completions emitters: [`examples/05_docs_completion.c`](examples/05_docs_completion.c)
- Env defaults & CLI overrides: [`examples/06_env_defaults.c`](examples/06_env_defaults.c)
- Sizes (IEC/SI) & pretty bytes: [`examples/07_sizes.c`](examples/07_sizes.c)

---

## Quick start

### Build & run examples (Meson)

```sh
# configure with examples enabled
meson setup build -Dbuildtype=debugoptimized -Dexamples=enabled
# build all examples
meson compile -C build examples
# run the curated example suite:
ninja -C build examples-run

# or try binaries manually:
./build/examples/ex-hello --name Ada --repeat 2
./build/examples/ex-remote remote add origin git@git.co
./build/examples/ex-sizes --limit 256MiB --rate 12MB file1 file2
```

### Run the unit tests

```sh
# Sanitized dev build (recommended)
meson setup build -Dbuildtype=debugoptimized \
  -Db_sanitize=address,undefined -Db_lundef=false
meson test -C build --print-errorlogs
```

> Tip: `meson test -C build --list` to see test names.
> You can also run the binary directly: `./build/cargs-tests`.

---

## Install (header + pkg-config)

```sh
meson setup build -Dbuildtype=release
meson install -C build            # installs include/c-args-parser/c-args-parser.h and cargs.pc
pkg-config --cflags cargs         # → -I…/include/c-args-parser
```

Then in your app:

```c
#include <c-args-parser/c-args-parser.h>
```

(If you prefer a shorter include, you can add a tiny wrapper `cargs.h` that includes `c-args-parser.h`.)

---

## Minimal example

```c
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <c-args-parser/c-args-parser.h>

typedef struct { int verbose; int jobs; uint64_t limit; } App;

static int on_verbose(const char* v, void* u){ (void)v; ((App*)u)->verbose=1; return CARGS_OK; }
static int on_jobs(const char* v, void* u){ int n=0; if(!v||cargs_read_int(v,&n)) return -1; ((App*)u)->jobs=n; return CARGS_OK; }
static int on_limit(const char* v, void* u){ uint64_t b=0; if(v && cargs_read_size_iec(v,&b)) return -1; ((App*)u)->limit=b; return CARGS_OK; }

static int run_root(int argc, char** argv, void* user){
  (void)argc; (void)argv; App* a=(App*)user;
  printf("verbose=%d jobs=%d limit=%" PRIu64 "B\n", a->verbose, a->jobs, a->limit);
  return CARGS_OK;
}

int main(int argc, char** argv){
  App app = {0};
  const cargs_opt opts[] = {
    { "verbose",'V', CARGS_ARG_NONE,     NULL, "Verbose",                 on_verbose, NULL,NULL, 0, CARGS_GRP_NONE },
    { "jobs",   'j', CARGS_ARG_REQUIRED, "N",  "Workers (env JOBS=4)",    on_jobs,    "JOBS","4", 0, CARGS_GRP_NONE },
    { "limit",  'l', CARGS_ARG_OPTIONAL, "SZ", "Limit (IEC; env LIM=1MiB)", on_limit, "LIM","1MiB", 0, CARGS_GRP_NONE },
  };
  const cargs_cmd root = { .name=NULL,.desc="demo",
    .opts=opts,.opt_count=3, .subs=NULL,.sub_count=0,.aliases=NULL,.alias_count=0,
    .pos=NULL,.pos_count=0, .run=run_root };

  cargs_env env = { .prog="demo", .version="c-args-parser 0.5.x", .author="You",
    .auto_help=true,.auto_version=true,.auto_author=true,.wrap_cols=90,.color=true,.out=stdout,.err=stderr };

  return cargs_dispatch(&env,&root,argc,argv,&app) < 0 ? 1 : 0;
}
```

Run it:
```sh
./demo -V -j 8 -l 256MiB
./demo --jobs=4 --limit -V
```

---

## Subcommands & positionals

Each command can have its own options + a **positional schema**:

```c
static const cargs_pos add_pos[] = { {"NAME",1,1}, {"URL",1,1} };
static int remote_add(int argc, char** argv, void* u){ (void)u;
  // argv[0] = NAME, argv[1] = URL
  printf("add %s %s\n", argv[0], argv[1]); return CARGS_OK;
}
static const cargs_cmd remote_subs[] = {
  { .name="add", .desc="Add",
    .pos=add_pos,.pos_count=2, .run=remote_add },
};
const cargs_cmd remote = { .name="remote", .desc="Manage remotes",
  .subs=remote_subs,.sub_count=1 };
```

Users can do: `prog remote add origin git@git.co`

See full example: [`examples/03_remote.c`](examples/03_remote.c)

---

## Option parsing rules (nuances)

- Long forms:
  - `--opt=VAL` and `--opt VAL` both work for required args.
- Short forms:
  - `-j10` (attached) and **`-j 10`** (separate) work for required args.
  - `-l12`, **`-l 12`**, and `-l -12` work for **optional** args (heuristic accepts a numeric-looking next token).
  - `-j=10` (equal sign with short options) is **not** supported (by design).
- End of options: `--` stops option parsing; everything after is positional.
- Negative numbers: required-arg options accept `-10`; optional‑arg options accept `-10` if the next token looks numeric.

---

## Groups (mutual exclusion & required-one)

- **XOR** (`CARGS_GRP_XOR`) — at most one of the flags in that group may be used.
- **REQ_ONE** (`CARGS_GRP_REQ_ONE`) — exactly one is required; env/defaults count too.

Example: [`examples/04_groups.c`](examples/04_groups.c)

---

## Env‑var defaults

Specify `.env` and `.def` on each option. At parse time:
1. The library first applies env/defaults (calls your callbacks).
2. Then it parses CLI; **CLI overrides env/defaults**.

Example: [`examples/06_env_defaults.c`](examples/06_env_defaults.c)

---

## Sizes & formatting

Helpers:
- `cargs_read_size_si("12MB",&b)` → KB/MB/GB = 1000-based.
- `cargs_read_size_iec("256MiB",&b)` → KiB/MiB/GiB = 1024-based.
- `cargs_fmt_bytes(bytes, buf, n, iec, decimals)` to pretty-print.

Example: [`examples/07_sizes.c`](examples/07_sizes.c)

---

## Auto docs & shell completions

From your command tree you can **emit**:

- **Markdown**: `cargs_emit_markdown(env, root, prog, FILE*)`
- **man(7)**: `cargs_emit_man(env, root, prog, FILE*, section)`
- **bash/zsh/fish** completions:
  - `cargs_emit_completion_bash(env, root, prog, FILE*)`
  - `cargs_emit_completion_zsh(env, root, prog, FILE*)`
  - `cargs_emit_completion_fish(env, root, prog, FILE*)`

See: [`examples/05_docs_completion.c`](examples/05_docs_completion.c)

---

## Meson tips

- Dev build with sanitizers:
  ```sh
  meson setup build -Dbuildtype=debugoptimized \
    -Db_sanitize=address,undefined -Db_lundef=false
  meson test -C build
  ```
- Hardened release build:
  ```sh
  meson setup build-release -Dbuildtype=release -Db_lto=true -Db_pie=true
  meson compile -C build-release
  ```
- `compile_commands.json` is generated in the build dir:
  ```sh
  meson setup build && meson compile -C build
  ls build/compile_commands.json
  ```

---

## API sketch

Key types:
- `cargs_opt` — describe an option (names, arg kind, help, callback, env/default, group).
- `cargs_pos` — positional schema (`min..max` occurrences).
- `cargs_cmd` — command node: name, desc, options, subcommands, aliases, positional schema, and `run` callback.
- `cargs_env` — global behavior (prog/version/author, colors, wrapping, I/O).

Entry point:
```c
int cargs_dispatch(const cargs_env *env, const cargs_cmd *root,
                   int argc, char **argv, void *user);
```
- Walks the tree, handles built-ins (`--help`, `--version`, `--author` if enabled), applies env/defaults, enforces groups, validates positionals, and calls the deepest command’s `.run()`.

---

## Design & guarantees

- **No allocations** in the library (except where you explicitly allocate in your callbacks).
- **No globals**, so it’s re‑entrant and thread‑safe if you keep your `user` state separate.
- Help output respects **`NO_COLOR`** and **`COLUMNS`**.
- Group IDs are small integers (1..32 typical; masks use 64‑bit internally).

---

## FAQ

**Q: Does it support `-j=10`?**
A: No; short options use `-j10` or `-j 10`. Long options support `--jobs=10` or `--jobs 10`.

**Q: Can subcommands have their own options and positionals?**
A: Yes — each level defines its own `opts[]` and `pos[]` arrays.

**Q: How do I stop option parsing?**
A: Use `--`. Everything after is treated as positional args, even if it starts with `-`.

**Q: Colors look weird in my CI.**
A: Set env var `NO_COLOR=1` or `.color=false` in `cargs_env`.

---

## Contributing

PRs welcome! Please run:
```sh
meson setup build -Dbuildtype=debugoptimized -Db_sanitize=address,undefined -Db_lundef=false
meson compile -C build && meson test -C build
```
Add tests under `tests/` and a focused example under `examples/` if you’re adding features.

---

## License

MIT. See `LICENSE`.
