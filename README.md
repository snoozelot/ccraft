# ccraft

Run C files as scripts. No Makefile. No autoconf. No CMake. No build infrastructure.

Write C with a shebang, execute directly. ccraft compiles on first run, caches the binary, recompiles when source changes. Edit, run, done.

```c
#!/usr/bin/env -S ccraft
#include <stdio.h>

int
main(int argc, char *argv[]) {
    printf("hello %s!\n", argc > 1 ? argv[1] : "world");
    return 0;
}
```

```sh
chmod +x hello.c
./hello.c Alice
# hello Alice!
```

With libraries via pkg-config:

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
./fetch.c https://example.com
```

## Usage

```
ccraft [CC_FLAG]... FILE_C [ARG]...
```

Flags:
- `-v` show compilation command
- `-vv` show compilation command and source code
- `-std=STD` override gnu99 as C standard
- `-o FILE` create persistent binary instead of ephemeral one
- `-p MODULE` link against pkg-config module
- any other CC flags (`-lm`, `-I...`, etc.)

Pipe from stdin:

```sh
cat hello.c | ccraft -vv /dev/stdin sam
```

## Caching

Set `CCRAFT_CACHE` environment variable:

- `mtime` (default) - recompile when source newer than binary
- `md5` - recompile when content hash changes (handles reverts)
- `none` - always recompile

The `hi-*.c` files are self-contained templates to embed in your project without ccraft dependency.

## Companion Tools

- `ccompile` — batch compile C files, chronic-style output (quiet on success)
- `clint` — lint ccraft scripts with gcc syntax-only mode
- `cproto` — extract function prototypes using clang AST

## Dependencies

- C compiler (`cc` or `$CC`)
- POSIX shell, `sed`, `md5sum`
- Optional: `bat` for syntax-highlighted source display

## Install

Copy `ccraft` to your PATH. See `examples/` for demos.
