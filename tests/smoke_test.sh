#!/usr/bin/env bash
# checks of the gauss program
# usage: bash tests/smoke_test.sh <path to gauss>
set -u

GAUSS="$(realpath "${1:?usage: smoke_test.sh <gauss>}")"
EXAMPLE="$(realpath "$(dirname "$0")/../examples/system4.txt")"
WORK="$(mktemp -d)"
PASS=0
FAIL=0

ok()  { PASS=$((PASS + 1)); printf '  ok   %s\n' "$1"; }
bad() { FAIL=$((FAIL + 1)); printf '  FAIL %s\n' "$1"; }
eq()  { if [ "$2" = "$3" ]; then ok "$1"; else bad "$1 (expected [$3], got [$2])"; fi; }

# largest Threads value in /proc/<pid>/status while the process lives
max_thread_count() {
    local most=0 now
    while now="$(awk '/^Threads:/ { print $2 }' "/proc/$1/status" 2> /dev/null)" && [ -n "$now" ]; do
        [ "$now" -gt "$most" ] && most="$now"
        sleep 0.01
    done
    echo "$most"
}

cd "$WORK"

# 1. example from a file, a11 = 0 so rows must be swapped
timeout 15 "$GAUSS" -t 1 < "$EXAMPLE" > out1.txt 2> err1.txt
eq "1.1 exit code 0" "$?" "0"
eq "1.2 the solution is 55/33 68/33 85/33 122/33" "$(tr '\n' ' ' < out1.txt)" \
   "1.666666667 2.060606061 2.575757576 3.696969697 "
eq "1.3 the residual is printed" "$(grep -c '^solved: n=4 threads=1' err1.txt)" "1"

# 2. same answer in 2, 3 and 8 threads
for T in 2 3 8; do
    timeout 15 "$GAUSS" -t "$T" < "$EXAMPLE" > "out2_$T.txt" 2> /dev/null
    RC=$?
    cmp -s out1.txt "out2_$T.txt" && [ "$RC" -eq 0 ] &&
        ok "2.$T $T threads give the same solution" ||
        bad "2.$T $T threads: code $RC, [$(tr '\n' ' ' < "out2_$T.txt")]"
done

# 3. input from a pipe and a single equation
eq "3.1 x + y = 3, x - y = 1 gives 2 1" \
   "$(printf '2\n1 1 3\n1 -1 1\n' | timeout 15 "$GAUSS" -t 2 2> /dev/null | tr '\n' ' ')" "2 1 "
eq "3.2 n = 1: 4x = 2 gives 0.5" "$(printf '1\n4 2\n' | timeout 15 "$GAUSS" -t 4 2> /dev/null)" "0.5"

# 4. no unique solution
printf '3\n1 2 3 6\n2 4 6 12\n1 1 1 3\n' | timeout 15 "$GAUSS" -t 2 > /dev/null 2> err4.txt
eq "4.1 dependent rows: code 5" "$?" "5"
eq "4.2 the reason is reported" "$(grep -c 'singular' err4.txt)" "1"
printf '2\n1 1 1\n1 1 2\n' | timeout 15 "$GAUSS" -t 2 > /dev/null 2>&1
eq "4.3 inconsistent rows: code 5" "$?" "5"
printf '2\n0 0 0\n0 0 0\n' | timeout 15 "$GAUSS" -t 3 > /dev/null 2>&1
eq "4.4 zero matrix: code 5" "$?" "5"

# 5. wrong input
printf '2\n1 x 3\n1 1 1\n' | timeout 15 "$GAUSS" -t 1 > /dev/null 2> err5.txt
eq "5.1 a word instead of a number: code 3" "$?" "3"
eq "5.2 the position is reported" "$(grep -c 'row 1, column 2' err5.txt)" "1"
printf '2\n1 1 2\n1 -1'    | timeout 15 "$GAUSS" -t 1 > /dev/null 2>&1
eq "5.3 the input is too short: code 3" "$?" "3"
printf '1\n1 1\n5\n'       | timeout 15 "$GAUSS" -t 1 > /dev/null 2>&1
eq "5.4 extra data after the system: code 3" "$?" "3"
printf '0\n'               | timeout 15 "$GAUSS" -t 1 > /dev/null 2>&1
eq "5.5 size 0: code 3" "$?" "3"
printf '1\nnan 1\n'        | timeout 15 "$GAUSS" -t 1 > /dev/null 2>&1
eq "5.6 nan: code 3" "$?" "3"

# 6. wrong keys
timeout 15 "$GAUSS" < "$EXAMPLE" > /dev/null 2> err6.txt
eq "6.1 no -t: code 2" "$?" "2"
eq "6.2 the usage is printed" "$(grep -c '^usage:' err6.txt)" "1"
I=3
for KEY in 0 -3 abc 4x 257; do
    timeout 15 "$GAUSS" -t "$KEY" < "$EXAMPLE" > /dev/null 2>&1
    eq "6.$I -t $KEY is rejected: code 2" "$?" "2"
    I=$((I + 1))
done
timeout 15 "$GAUSS" -t 2 -x < "$EXAMPLE" > /dev/null 2>&1
eq "6.8 unknown key: code 2" "$?" "2"
timeout 15 "$GAUSS" -t 2 extra < "$EXAMPLE" > /dev/null 2>&1
eq "6.9 extra argument: code 2" "$?" "2"

# 7. write error, /dev/full is always full
timeout 15 "$GAUSS" -t 2 < "$EXAMPLE" > /dev/full 2> err7.txt
eq "7.1 write error: code 4" "$?" "4"
eq "7.2 the reason is reported" "$(grep -c '^stdout: ' err7.txt)" "1"

# 8. random 300 x 300 system, same result for any number of threads
timeout 60 "$GAUSS" -t 1 -g 300 2> err8_1.txt
eq "8.1 one thread: code 0" "$?" "0"
RESIDUAL="$(grep -oE 'residual=[0-9.e+-]+' err8_1.txt | cut -d= -f2)"
awk -v r="$RESIDUAL" 'BEGIN { exit !(r < 1e-9) }' && ok "8.2 residual $RESIDUAL < 1e-9" ||
    bad "8.2 residual [$RESIDUAL] is too big"
I=3
for T in 2 4 7 16; do
    timeout 60 "$GAUSS" -t "$T" -g 300 2> "err8_$T.txt"
    eq "8.$I $T threads: the same residual" \
       "$(grep -oE 'residual=[0-9.e+-]+' "err8_$T.txt" | cut -d= -f2)" "$RESIDUAL"
    I=$((I + 1))
done

# 9. every thread prints its own tid, thread 0 is the main one
timeout 15 "$GAUSS" -t 5 -g 50 2> err9.txt
eq "9.1 5 threads report themselves" "$(grep -c 'thread [0-4]/5 tid=' err9.txt)" "5"
eq "9.2 the 5 TIDs are different" \
   "$(grep -oE 'thread [0-9]+/5 tid=[0-9]+' err9.txt | cut -d= -f2 | sort -u | wc -l)" "5"
PID="$(grep -oE 'pid=[0-9]+' err9.txt | head -1 | cut -d= -f2)"
eq "9.3 thread 0 is the main thread" "$(grep -oE 'thread 0/5 tid=[0-9]+' err9.txt | cut -d= -f2)" "$PID"

# 10. the os sees exactly -t threads
I=1
for T in 1 4 6; do
    "$GAUSS" -t "$T" -g 1500 2> /dev/null &
    JOB=$!
    MOST="$(max_thread_count "$JOB")"
    wait "$JOB"
    RC=$?
    eq "10.$I -t $T: at most $T threads in /proc, code 0" "$MOST:$RC" "$T:0"
    I=$((I + 1))
done

# 11. pthread_create fails because of ulimit, the program must not hang
timeout 15 bash -c 'ulimit -u 1; exec "$0" -t 4 -g 50' "$GAUSS" > /dev/null 2> err11.txt
eq "11.1 pthread_create fails: code 6" "$?" "6"
eq "11.2 the reason is reported" "$(grep -c '^pthread_create: ' err11.txt)" "1"

# 12. typing by hand, script gives the program a terminal
printf '2\n1 1 3\n1 -1 1\n' |
    timeout 15 script -q -e -c "$GAUSS -t 2" /dev/null > out12.txt 2>&1
eq "12.1 exit code 0 without EOF after the last row" "$?" "0"
eq "12.2 the size and both rows are asked for" \
   "$(grep -o 'size of the system n:\|row [12] of 2' out12.txt | wc -l)" "3"
eq "12.3 the solution 2 1 is printed" \
   "$(tr -d '\r' < out12.txt | grep -xE '2|1' | tail -2 | tr '\n' ' ')" "2 1 "

echo
echo "TOTAL: passed $PASS, failed $FAIL"
cd /
rm -rf "$WORK"
[ "$FAIL" -eq 0 ]
