#!/usr/bin/env bash
# shows the threads of a running gauss with ps, /proc and top
# usage: bash tests/threads_demo.sh <path to gauss> [threads] [size]
set -u

GAUSS="$(realpath "${1:?usage: threads_demo.sh <gauss> [threads] [size]}")"
T="${2:-4}"
N="${3:-3000}"

echo "\$ ./gauss -t $T -g $N &"
"$GAUSS" -t "$T" -g "$N" &
PID=$!

# wait until the threads are started
for _ in $(seq 1 300); do
    [ "$(awk '/^Threads:/ { print $2 }' "/proc/$PID/status" 2> /dev/null)" = "$T" ] && break
    sleep 0.01
done
sleep 0.5

echo
echo "\$ ps -L -o pid,lwp,nlwp,psr,stat,comm -p $PID"
LINES=50 COLUMNS=100 ps -L -o pid,lwp,nlwp,psr,stat,comm -p "$PID"
echo
echo "\$ grep Threads /proc/$PID/status"
grep Threads "/proc/$PID/status"
echo
echo "\$ ls /proc/$PID/task"
ls "/proc/$PID/task" | tr '\n' ' '
echo
echo
echo "\$ top -H -b -n 1 -p $PID | tail -n +7"
top -H -b -n 1 -p "$PID" | tail -n +7
wait "$PID"
echo
echo "exit code: $?"
