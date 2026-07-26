# ccraft

C development tools.

## Tools

| Tool | Purpose |
|------|---------|
| [`ccraft`](ccraft) | Run C files as scripts |
| [`ccompile`](ccompile) | Batch compile, quiet on success |
| [`clint`](clint) | Lint C scripts |
| [`goo`](goo) | Run Go files as scripts |
| [`goon`](goon) | Discover and run Go projects |
| [`cproto`](cproto) | Extract function prototypes |
| [`cdecl`](cdecl) | Explain C declarations |
| [`cflow`](cflow) | Function call tree |
| [`cindent`](cindent) | Format to 1TBS style |
| [`draft/cinclude`](draft/cinclude) | Include dependency tree |
| [`draft/cdeadl`](draft/cdeadl) | Detect deadlocks |
| [`draft/cstruct`](draft/cstruct) | Show struct layout with padding |
| [`draft/cxref`](draft/cxref) | Cross-reference symbols |
| [`reef/tt.h`](reef/tt.h) | Minimal C test framework |

## ccraft

Run C files as scripts. No Makefile, no CMake — shebang and execute.
Compiles on first run, caches in `/tmp/`, recompiles when source changes.
`-R` fetches remote headers, `// ccraft` annotations embed extra flags.

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

Remote deps via `-R`:

```sh
$ ccraft -R https://raw.githubusercontent.com/sheredom/utf8.h/main/utf8.h file.c
```

Annotations embed flags in the source (after shebang), includes reference `"nugget/..."`:

```c
// ccraft -p freetype2 -R https://raw.githubusercontent.com/sheredom/utf8.h/main/utf8.h
#include FT_FREETYPE_H
#include "nugget/utf8.h"
```

## ccompile

Batch compile C files, quiet on success. Binary next to source unless `-d DIR` or `-o FILE`.

```sh
$ ccompile src/*.c              # src/foo → src/foo
$ ccompile -d build src/*.c     # src/foo → build/foo
$ ccompile -o server src/*.c    # single binary
```

Flags embedded on line 1:

```c
// ccompile -lm -DDEBUG
```

## reef/tt.h

```c
// ccraft -R https://raw.githubusercontent.com/snoozelot/ccraft/master/reef/tt.h
#include "nugget/tt.h"

TEST(math) {
    ASSERT_EQ("two_plus_two", 2 + 2, 4);
}

int main(int argc, char **argv) {
    return tt_main(argc, argv);
}
```

The `hi-*.c` and `hi-*.go` files are standalone templates (bash wrapper + source, no ccraft/goo needed).

## clint

Lint C scripts via gcc syntax-only mode.

```sh
$ clint script.c
```

## goo

Run a single `.go` file as a script. No go.mod needed.
Ephemeral module, resolves `// go get` deps and `-p`, caches in `/tmp/`.

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
$ cat script.go | goo /dev/stdin
```

Local packages in the source's directory importable as `goo/PACKAGE`. `-I DIR` adds more.
Cache: `GOO_CACHE=mtime|md5|none`.

## goon

Run a Go project from any subdirectory. Finds `go.mod` upward, discovers `main()`, tidies on rebuild, caches in `/tmp/`.

```sh
$ goon .                      # discover and run
$ goon -v .                   # show build commands
$ goon -m main.go             # file as entry point
$ goon -o /tmp/app .          # write binary
```

Cache: `GOON_CACHE=mtime|md5|none`.

The `hi-*.go` files are standalone templates (no goo/goon needed).

## cproto

Extract function prototypes and types via clang AST. Generate headers with guards.

```sh
$ cproto src/parser.c                   # prototypes only
$ cproto -T src/parser.c                # + structs/enums/typedefs
$ cproto -TgfS src/parser.c > parser.h  # full header
```

Options: `-s` static, `-T` types, `-g` guards, `-f` forward decls, `-S` sort by deps.

## cdecl

Explain C declarations in plain English.

```sh
$ cdecl 'int (*fp)(int, char)'
fp is pointer to function (int, char) returning int
```

## cflow

Call tree. Forward from root, `-R` for reverse (find callers).

```sh
$ cflow examples/cflow.c
main()
    factorial() [recursive]
    printf()
    process()
        ...

$ cflow -R validate examples/cflow.c
validate()
    process()
        main()
```

## cindent

Format C to 1TBS (4-space, brace on same line). clang-format by default, `-g` for GNU indent.

```sh
$ cindent file.c          # stdout
$ cindent -i src/*.c      # in place
$ cat foo.c | cindent     # stdin
```

## draft/cinclude

Include dependency tree.

```sh
$ draft/cinclude -s -d 2 file.c
```

## draft/cdeadl

Detect deadlocks from lock acquisition order.

```sh
$ draft/cdeadl file.c
deadlock: Account.mu vs Config.mu
  Account.mu:54 → Config.mu:55
  Config.mu:92 → Account.mu:93
```

## draft/cstruct

Annotate struct memory layout with sizes, offsets, padding.

```sh
$ draft/cstruct file.c struct_name
```

## draft/cxref

Cross-reference symbols: definition and usage locations.

```sh
$ draft/cxref file.c
validate   function   file.c   20   declaration
validate   function   file.c   26   definition
validate   function   file.c   40   reference
```
