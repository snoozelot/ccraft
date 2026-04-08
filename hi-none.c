#!/usr/bin/env bash
# hi-none.c - Always recompile (no caching)
FILE="${0}"
CC_FLAGS=(-std=gnu99 -O0 -pipe -I"$(dirname "$(readlink -f "${FILE}")")" -Wall -Wextra -Wno-unused)
TEMP_C="$(mktemp "${TMPDIR-/tmp}/XXX.c")"
OUT="${TEMP_C%.c}"
trap 'rm -f -- "${TEMP_C}" "${OUT}"' EXIT
sed -e "1s|^#!.*|#line 2 \"${FILE}\"|" -e '2,/^#!/ s|.*||' < "${FILE}" > "${TEMP_C}"

C_LINE=$( sed -n '1n; /^#!/{=;q}' "${FILE}" )
bat --language=C --number --paging=never --line-range="$(( C_LINE + 1 )):" "${TEMP_C}" 1>&2 2>/dev/null ||
cat -n "${TEMP_C}" | tail -n +$(( C_LINE + 1 )) 1>&2 2>/dev/null

set -x
"${CC-cc}" "${CC_FLAGS[@]}" -o"${OUT}" "${TEMP_C}" 1>&2
(exec -a "${FILE}" "${OUT}" "${@}")
exit $?
#!/usr/bin/env -S ccraft # -----------------------------------------------------
#include <stdio.h>

int
main(int argc, char *argv[]) {
    printf("hello %s!\n", argc > 1 ? argv[1] : "world");
    return 0;
}
