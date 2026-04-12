# ccraft

C development tools.

## Tools

- [ccraft](#ccraft-1) — run C files as scripts
- [ccompile](#ccompile) — batch compile, quiet on success
- [clint](#clint) — lint C scripts
- [cproto](#cproto) — extract function prototypes
- [cdecl](#cdecl) — explain C declarations
- [cflow](#cflow) — function call tree
- [cindent](#cindent) — format to 1TBS style
- [draft/cinclude](#draftcinclude) — include dependency tree
- [draft/cdeadl](#draftcdeadl) — detect deadlocks
- [draft/cstruct](#draftcstruct) — show struct layout with padding
- [draft/cxref](#draftcxref) — cross-reference symbols

## ccraft

Run C files as scripts. No Makefile, no autoconf, no CMake. Write, execute.
Compiles on first run, caches the binary, recompiles when source changes.

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

CC_FLAGs:
  -v          Show compilation command
  -vv         Show compilation command and source code
  -std=STD    Override gnu99 as C standard with STD
  -o FILE     Create persistent binary FILE instead of ephemeral one
  -p MODULE   Link against pkg-config MODULE, e.g. -p freetype2
  ...         Any other flags supported by CC

Environment:
  CC            = cc
  CCRAFT_CACHE  = mtime  (mtime|md5|none)

Caching:
  mtime   Recompile when source newer than binary (default)
  md5     Recompile when content hash changes
  none    Always recompile
```

The `hi-*.c` files are self-contained templates to embed in your project without ccraft dependency.

## ccompile

Batch compile C files. Quiet on success (chronic-style), shows output only on failure. Binary goes next to source.

```sh
$ ccompile src/*.c              # src/foo.c → src/foo
$ ccompile -d build src/*.c     # src/foo.c → build/foo
```

## clint

Lint C scripts using gcc syntax-only mode.

```sh
$ clint script.c
```

## cproto

Extract function prototypes from C source using clang AST.

```sh
$ cproto src/parser.c
```

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

```sh
$ cindent examples/cindent.c           # to stdout
$ cindent -i src/*.c                   # in place
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

- C compiler (`cc` or `$CC`)
- POSIX shell, `sed`, `md5sum`, `gawk`
- Analysis tools: `clang`, `jq`
- Formatting: `clang-format` or `indent`
