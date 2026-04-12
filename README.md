# ccraft

C development tools.

## Tools

- [ccraft](#ccraft-1) — run C files as scripts
- [ccompile](#ccompile) — batch compile, quiet on success
- [clint](#clint) — lint C scripts
- [cproto](#cproto) — extract function prototypes
- [cdeadl](#cdeadl) — detect deadlocks
- [cdecl](#cdecl) — explain C declarations
- [cflow](#cflow) — function call tree
- [cindent](#cindent) — format to 1TBS style

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

## cdeadl

Detect potential deadlocks by analyzing lock acquisition order across functions.

```sh
$ cdeadl examples/deadlock.c
deadlock: Account.mu vs Config.mu
  Account.mu:54 → Config.mu:55
  Config.mu:92 → Account.mu:93

deadlock: Account.mu → Logger.mu → Config.mu → Account.mu
  Account.mu:72 → Logger.mu:73
  ...
```

## cdecl

Explain C declarations in plain English.

```sh
$ cdecl 'int (*fp)(int, char)'
fp is pointer to function (int, char) returning int

$ cat examples/declarations.txt | cdecl
```

## cflow

Show function call tree. Forward from a root, or reverse to find callers.

```sh
$ cflow examples/calltree.c
main() <examples/calltree.c:56>
    factorial() <examples/calltree.c:47>
        factorial() [recursive]
    printf()
    process() <examples/calltree.c:38>
        ...

$ cflow -R validate examples/calltree.c
validate() <examples/calltree.c:26>
    process() <examples/calltree.c:38>
        main() <examples/calltree.c:56>
```

## cindent

Format C code to 1TBS style (4-space indent, braces on same line).

```sh
$ cindent examples/unformatted.c       # to stdout
$ cindent -i src/*.c                   # in place
```

## Dependencies

- C compiler (`cc` or `$CC`)
- POSIX shell, `sed`, `md5sum`
- Analysis tools: `clang`, `jq`
- Formatting: `clang-format` or `indent`
