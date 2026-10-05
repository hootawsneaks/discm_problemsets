#!/bin/bash
# Runs all four variants at several thread counts and writes results.csv
# Usage: bash run_all.sh            (defaults below)
#        RANGE=1000000 RUNS=5 THREADS="0 1 2 4 8" bash run_all.sh
# Each variant folder needs a compiled ./main (see compile instructions).
# Output of the program is sent to /dev/null so the terminal doesn't skew timings;
# only the "Elapsed" line is kept.

RANGE=${RANGE:-100000}
RUNS=${RUNS:-3}
THREADS=${THREADS:-"0 1 2 4 8 16 32"}
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/results.csv"
VARIANTS="print-immediately_straight-division print-immediately_divisibility-threads print-at-end_straight-division print-at-end_divisibility-threads"

echo "variant,range,threads,run,elapsed_ms" > "$OUT"
for v in $VARIANTS; do
  if [ ! -x "$ROOT/$v/main" ]; then echo "skipping $v (no ./main, compile it first)"; continue; fi
  for t in $THREADS; do
    printf "threads %s\nrange %s" "$t" "$RANGE" > "$ROOT/$v/config.txt"
    for r in $(seq 1 "$RUNS"); do
      ms=$(cd "$ROOT/$v" && ./main | grep "Elapsed" | awk '{print $2}')
      echo "$v,$RANGE,$t,$r,$ms" >> "$OUT"
      echo "$v threads=$t run=$r: ${ms} ms"
    done
  done
  # leave a sane config behind
  printf "threads 4\nrange %s" "$RANGE" > "$ROOT/$v/config.txt"
done
echo; echo "Averages (ms):"
awk -F, 'NR>1 && $5!="" {k=$1" threads="$3; s[k]+=$5; n[k]++} END {for (k in s) printf "%-70s %.2f\n", k, s[k]/n[k]}' "$OUT" | sort
echo; echo "Saved to $OUT"
