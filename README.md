# ccraft

C development tools.

## Tools

- [ccraft](#ccraft-1) — run C files as scripts
- [ccompile](#ccompile) — batch compile, quiet on success
- [clint](#clint) — lint C scripts
- [goo](#goo) — run Go files as scripts
- [goon](#goon) — discover and run Go projects
- [cproto](#cproto) — extract function prototypes
- [cdecl](#cdecl) — explain C declarations
- [cflow](#cflow) — function call tree
- [cindent](#cindent) — format to 1TBS style
- [draft/cinclude](#draftcinclude) — include dependency tree
- [draft/cdeadl](#draftcdeadl) — detect deadlocks
- [draft/cstruct](#draftcstruct) — show struct layout with padding
- [draft/cxref](#draftcxref) — cross-reference symbols

## ccraft

Run C files as scripts. No Makefile, no autoconf, no CMake, just add shebang and execute.
Compiles on first run, caches the binary in /tmp/, recompiles when source changes.

```c
#!/usr/bin/env -S ccraft -p libcurl
#include <curl/curl.h>

int
main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return 0;
}
```

```sh
$ chmod +x fetch.c
$ ./fetch.c https://example.com
```

```
$ ccraft -h
Usage: ccraft [CC_FLAG]... FILE_C [ARG]...

Compile FILE_C and execute it with ARGs as arguments to the program.
Caches binary in /tmp/ — silent on hit, rebuilds when source changes.
When FILE_C is /dev/stdin, cwd is added as include path.
When FILE_C is a regular file, its directory is added as include path.

CC_FLAGs:
  -v          Show compilation command
  -vv         Show compilation command and source code (rebuild only)
  -std=STD    Override gnu99 as C standard with STD
  -o FILE     Write binary to FILE instead of /tmp/ cache
  -p MODULE   Link against pkg-config MODULE, e.g. -p freetype2
  -lLIB       Link library (compact form, e.g. -lm)
  ...         Any other CC/LD flags

Environment:
  CC            = cc
  CCRAFT_CACHE  = mtime  (mtime|md5|none)
  TMPDIR        = /tmp (override for temp files)

Caching:
  mtime   Recompile when source newer than binary (default)
  md5     Recompile when content hash changes
  none    Always recompile
```

The `hi-*.c` and `hi-*.go` files are self-contained templates to embed in
your project without depending on the ccraft/goo toolset. Each file has a
bash wrapper that handles compilation and a cache strategy (mtime, md5, none).

## ccompile

Batch compile C files. Quiet on success (chronic-style), shows output only on failure. Binary goes next to source.
Per-file compiler flags can be embedded on line 1 (shebang or comment).

```sh
$ ccompile src/*.c              # src/foo.c → src/foo
$ ccompile -d build src/*.c     # src/foo.c → build/foo
$ ccompile -o server src/*.c    # single binary
```

```c
// ccompile -lm -DDEBUG
#include <math.h>
int main() { return (int)sqrt(4) - 2; }
```

## clint

Lint C scripts using gcc syntax-only mode.

```sh
$ clint script.c
```

## goo

Run a single `.go` file as a script. No go.mod needed.

Creates an ephemeral module, resolves `// go get` directives and `-p` deps,
caches the binary in `/tmp/`. Shebang support: `./script.go` works.

```go
#!/usr/bin/env -S goo
// go get rsc.io/quote@v1.5.2

package main

import "rsc.io/quote"

func main() {
    println(quote.Hello())
}
```

```sh
$ chmod +x hello.go
$ ./hello.go
$ goo hello.go                # same effect
$ goo -v hello.go             # show build commands
$ goo -p rsc.io/quote@v1.5.2 hello.go  # dep from flag
$ cat script.go | goo /dev/stdin
```

Local packages in the source file's directory are importable as `goo/PACKAGE`.
Use `-I DIR` to add more directories:

```sh
$ goo -I ../lib script.go     # import "goo/mylib"
```

Cache strategies: `GOO_CACHE=mtime` (default), `md5`, `none`.

See `goo -h` for full options.

## goon

Run a Go project from any subdirectory. No need to remember `./cmd/server` paths.

Walks up from DIR to find `go.mod`, discovers the `main()` package, runs
`go mod tidy` on rebuild, caches the binary in `/tmp/`.

```sh
$ goon .                      # discover and run from current dir
$ goon -v .                   # show build commands
$ goon ./cmd/server           # explicit subdirectory
$ goon -m main.go             # file as project entry point
$ goon -o /tmp/myapp .        # write binary to specific path
```

Cache strategies: `GOON_CACHE=mtime` (default), `md5`, `none`.

See `goon -h` for full options.

The `hi-*.go` files are self-contained templates (bash wrapper + Go source
in one file), one per cache strategy. Copy, chmod, run — no goo/goon needed.

## cproto

Extract function prototypes and type definitions from C source using clang AST.
Generate complete header files with guards, forward declarations, and dependency-sorted types.

```sh
$ cproto src/parser.c                   # function prototypes only
$ cproto -T src/parser.c                # include structs/enums/typedefs
$ cproto -TgfS src/parser.c > parser.h  # full header with guards
```

Options: `-s` static functions, `-T` types, `-g` guards, `-f` forward decls, `-S` sort by deps.

## cdecl

Explain C declarations in plain English.

```sh
$ cdecl 'int (*fp)(int, char)'
fp is pointer to function (int, char) returning int

$ cat examples/cdecl.txt | cdecl
```

## cflow

Show function call tree. Forward from a root, or reverse to find callers.

```sh
$ cflow examples/cflow.c
main() <examples/cflow.c:56>
    factorial() <examples/cflow.c:47>
        factorial() [recursive]
    printf()
    process() <examples/cflow.c:38>
        ...

$ cflow -R validate examples/cflow.c
validate() <examples/cflow.c:26>
    process() <examples/cflow.c:38>
        main() <examples/cflow.c:56>
```

## cindent

Format C code to 1TBS style (4-space indent, braces on same line).
Uses clang-format by default, or GNU indent with `-g`.

```sh
$ cindent examples/cindent.c           # to stdout
$ cindent -i src/*.c                   # in place
$ cindent -g examples/cindent.c        # GNU indent backend, to stdout
$ cat foo.c | cindent                  # stdin to stdout
```

## draft/cinclude

Show include dependency tree.

```sh
$ draft/cinclude -s -d 2 examples/cflow.c
examples/cflow.c
<stdio.h>                               → /usr/include/stdio.h
    <bits/libc-header-start.h>          → /usr/include/bits/libc-header-start.h
    <bits/types.h>                      → /usr/include/bits/types.h
    ...
<stdlib.h>                              → /usr/include/stdlib.h
    ...
```

## draft/cdeadl

Detect potential deadlocks by analyzing lock acquisition order across functions.

```sh
$ draft/cdeadl examples/cdeadl.c
deadlock: Account.mu vs Config.mu
  Account.mu:54 → Config.mu:55
  Config.mu:92 → Account.mu:93

deadlock: Account.mu → Logger.mu → Config.mu → Account.mu
  Account.mu:72 → Logger.mu:73
  ...
```

## draft/cstruct

Annotate C source with struct memory layout. Shows field sizes, offsets, and padding waste.

```sh
$ draft/cstruct examples/cstruct.c packet
struct packet {                         /* 24 bytes, align 8 */
    char type;                          /*  1 byte  @  0 bytes, 7 padding */
    void *data;                         /*  8 bytes @  8 bytes */
    short len;                          /*  2 bytes @ 16 bytes */
    char flags;                         /*  1 byte  @ 18 bytes, 5 padding */
};
```

## draft/cxref

Cross-reference symbols: where every function, variable, type is defined and used.

```sh
$ draft/cxref examples/cflow.c
validate   function   examples/cflow.c   20   declaration
validate   function   examples/cflow.c   26   definition
validate   function   examples/cflow.c   40   reference
...
```

## Dependencies

- C compiler, `cc` or `$CC`
- POSIX shell, `sed`, `md5sum`, `gawk`
- `clang`, `jq` (cproto, cflow, cinclude, cdeadl, cstruct, cxref)
- `clang-format` or `indent` (cindent)
- `gcc` (clint)
- `go` (goo, goon)
- `pkg-config` (ccraft -p)
