#!/usr/bin/env bash
# time, speedup and efficiency for several sizes and thread counts
# usage: bash tests/bench.sh <path to gauss>
# every point is the median of REPEATS runs
set -eu

GAUSS="$(realpath "${1:?usage: bench.sh <gauss>}")"
SIZES="${SIZES:-100 200 500 1000 2000 3000}"
THREADS="${THREADS:-1 2 3 4 6 8 12 16}"
REPEATS="${REPEATS:-5}"

median() {
    printf '%s\n' "$@" | sort -g | awk '{ v[NR] = $1 } END { print v[int((NR + 1) / 2)] }'
}

echo "# $(lscpu | sed -n 's/^Model name: *//p'), $(nproc) logical CPUs"
echo "# $(uname -sr); median of $REPEATS runs"
printf '%-6s %-8s %-12s %-8s %s\n' n threads time_ms speedup efficiency

for N in $SIZES; do
    BASE=""
    for P in $THREADS; do
        TIMES=()
        for _ in $(seq 1 "$REPEATS"); do
            LINE="$("$GAUSS" -t "$P" -g "$N" 2>&1 | grep '^solved:')"
            TIMES+=("$(echo "$LINE" | grep -oE 'time=[0-9.]+' | cut -d= -f2)")
        done
        TIME="$(median "${TIMES[@]}")"
        [ -z "$BASE" ] && BASE="$TIME"
        awk -v n="$N" -v p="$P" -v t="$TIME" -v b="$BASE" 'BEGIN {
            printf "%-6d %-8d %-12.3f %-8.3f %.3f\n", n, p, t, b / t, b / t / p
        }'
    done
done
