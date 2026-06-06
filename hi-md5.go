#!/usr/bin/env bash
# hi-md5.go — Go script runner, content hash cache
FILE=$(readlink -f "${0}") || exit 1
BASE="${FILE##*/}"
HASH=$({ id -u; cat "${FILE}"; } | md5sum | head -c8)
OUT="${TMPDIR-/tmp}/${BASE%.go}-${HASH}"

if [[ ! -x "${OUT}" ]]; then
    DIR=$(mktemp -d "/tmp/${BASE%.go}-XXXXX")
    trap 'rm -rf "${DIR}"' EXIT
    sed -e '1s|#!|//|' -e '2,/^#!/ s|.*||' < "${FILE}" > "${DIR}/main.go"

    CLINE=$(sed -n '1n; /^#!/{=;q}' "${FILE}")
    bat --language=go --number --paging=never --line-range="$(( CLINE + 1 )):" "${DIR}/main.go" 2>/dev/null ||
    cat -n "${DIR}/main.go" | tail -n +"$(( CLINE + 1 ))" 2>/dev/null

    cd "${DIR}" || exit 1
    set -x
	if [[ -o xtrace ]]; then "${GO-go}" mod init "${BASE%.go}";              else "${GO-go}" mod init "${BASE%.go}" 2>/dev/null;              fi
	if [[ -o xtrace ]]; then "${GO-go}" mod tidy 2>&1;                       else "${GO-go}" mod tidy 2>/dev/null;                            fi
	if [[ -o xtrace ]]; then "${GO-go}" build -buildvcs=false -o "${OUT}" .; else "${GO-go}" build -buildvcs=false -o "${OUT}" . 2>/dev/null; fi || exit 1
fi

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
