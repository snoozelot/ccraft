#!/usr/bin/env bash
# hi-none.go — Go script runner, always rebuild
FILE=$(readlink -f "${0}") || exit 1
BASE="${FILE##*/}"
DIR=$(mktemp -d "/tmp/${BASE%.go}-XXXXX")
OUT=$(mktemp "/tmp/${BASE%.go}-XXXXX")
trap 'rm -rf "${DIR}" "${OUT}"' EXIT

sed -e '1s|#!|//|' -e '2,/^#!/ s|.*||' < "${FILE}" > "${DIR}/main.go"

CLINE=$(sed -n '1n; /^#!/{=;q}' "${FILE}")
bat --language=go --number --paging=never --line-range="$(( CLINE + 1 )):" "${DIR}/main.go" 2>/dev/null ||
cat -n "${DIR}/main.go" | tail -n +"$(( CLINE + 1 ))" 2>/dev/null

cd "${DIR}"
set -x
"${GO-go}" mod init "${BASE%.go}" 2>/dev/null
"${GO-go}" mod tidy 2>&1
"${GO-go}" build -buildvcs=false -o "${OUT}" . 1>&2

(exec -a "${FILE}" "${OUT}" "${@}")
exit $?
#!/usr/bin/env -S goon
package main

import (
    "fmt"
    "rsc.io/quote"
    "os"
)

func main() {
    arg := "world"
    if len(os.Args) > 1 {
        arg = os.Args[1]
    }
    fmt.Printf("hello %s!\n%s\n", arg, quote.Hello())
}
