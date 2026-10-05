#!/bin/bash
# Edge-case checks for all four variants. Usage: bash edge_cases.sh
# Each variant needs a compiled ./main. Checks prime counts against a reference
# sieve for valid configs, and shows the first output line for invalid ones.
ROOT="${ROOT:-$(cd "$(dirname "$0")" && pwd)}"
VARIANTS="print-immediately_straight-division print-immediately_divisibility-threads print-at-end_straight-division print-at-end_divisibility-threads"

pi() { awk -v n="$1" 'BEGIN{c=0; for(i=2;i<=n;i++){p=1; for(j=2;j*j<=i;j++) if(i%j==0){p=0;break} c+=p} print c}'; }

count_primes() { # $1 = output file
  grep -v -E '^(Start|End|Elapsed)' "$1" | if grep -q "found prime" "$1"; then grep -c "found prime"; else tr ' ' '\n' | grep -c -E '^[0-9]+$'; fi
}

# threads range
CASES="0 0|0 1|0 2|0 3|0 4|0 5|1 2|1 3|1 4|1 10|4 100|7 100|100 10|1000 50|5 1|0 1000|3 10000"

for v in $VARIANTS; do
  [ -x "$ROOT/$v/main" ] || { echo "skipping $v (no ./main)"; continue; }
  echo "== $v"
  W=$(mktemp -d); cp "$ROOT/$v/main" "$W/"
  IFS='|'; for c in $CASES; do
    unset IFS; set -- $c; t=$1; r=$2
    printf "threads %s\nrange %s" "$t" "$r" > "$W/config.txt"
    (cd "$W" && ./main > out.txt 2>&1); rc=$?
    got=$(count_primes "$W/out.txt"); exp=$(pi "$r")
    [ "$got" = "$exp" ] && s=PASS || s=FAIL
    echo "  $s threads=$t range=$r expected=$exp got=$got exit=$rc"
  done; unset IFS
  # invalid configs: show first line of output
  for cfg in "threads -1\nrange 100" "threads 4\nrange -5" "threads abc\nrange 100" "threads 4" "range 100" ""; do
    printf "$cfg" > "$W/config.txt"
    (cd "$W" && ./main > out.txt 2>&1); rc=$?
    echo "  INVALID cfg=[$(printf "$cfg" | tr '\n' ' ')] exit=$rc first_line=[$(head -c 60 "$W/out.txt" | head -1)]"
  done
  rm -f "$W/config.txt"; (cd "$W" && ./main 2>&1 | head -1 | sed 's/^/  NO CONFIG FILE first_line=[/; s/$/]/')
  rm -rf "$W"
done
