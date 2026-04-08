#!/usr/bin/env bash
# hi-mtime.c - Recompile when source is newer than binary
FILE="${0}"
BASE="${FILE##*/}"
SCOPE_ID=$({ id -u; readlink -f "${FILE}"; } | md5sum | head -c8)
TEMP_C="${TMPDIR-/tmp}/${BASE%.c}-${SCOPE_ID}.c"
OUT="${TEMP_C%.c}"
CC_FLAGS=(-std=gnu99 -O0 -pipe -I"$(dirname "$(readlink -f "${FILE}")")" -Wall -Wextra -Wno-unused)

if [[ ! -x "${OUT}" ]] || [[ "${FILE}" -nt "${OUT}" ]]; then
    trap 'rm -f -- "${TEMP_C}"' EXIT
    sed -e "1s|^#!.*|#line 2 \"${FILE}\"|" -e '2,/^#!/ s|.*||' < "${FILE}" > "${TEMP_C}"

    C_LINE=$( sed -n '1n; /^#!/{=;q}' "${FILE}" )
    bat --language=C --number --paging=never --line-range="$(( C_LINE + 1 )):" "${TEMP_C}" 1>&2 2>/dev/null ||
    cat -n "${TEMP_C}" | tail -n +$(( C_LINE + 1 )) 1>&2 2>/dev/null

    set -x
    "${CC-cc}" "${CC_FLAGS[@]}" -o"${OUT}" "${TEMP_C}" 1>&2
fi

(exec -a "${FILE}" "${OUT}" "${@}")
exit $?
#!/usr/bin/env -S ccraft # -----------------------------------------------------
#include <stdio.h>

int
main(int argc, char *argv[]) {
    printf("hello %s!\n", argc > 1 ? argv[1] : "world");
    return 0;
}
