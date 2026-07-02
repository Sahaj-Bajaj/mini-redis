#!/usr/bin/env bash
# Measures raw round-trip latency for a single PING using bash timing.
# Usage: ./scripts/latency.sh [samples]
# Produces a sorted list of µs latencies for quick sanity-checking.

set -euo pipefail

SAMPLES=${1:-100}
PORT=6380
declare -a TIMES

echo "Sampling PING latency ($SAMPLES samples)..."

for i in $(seq 1 "$SAMPLES"); do
    start=$(date +%s%N)
    echo -e "PING\r" | nc -q1 localhost $PORT > /dev/null
    end=$(date +%s%N)
    TIMES+=($((( end - start ) / 1000)))  # ns → µs
done

# Sort and print percentiles via awk.
printf '%s\n' "${TIMES[@]}" | sort -n | awk -v n="$SAMPLES" '
{
    a[NR] = $1
}
END {
    p50  = a[int(n * 0.50)]
    p99  = a[int(n * 0.99)]
    p999 = a[int(n * 0.999) > 0 ? int(n * 0.999) : n]
    print "p50  = " p50  " µs"
    print "p99  = " p99  " µs"
    print "p99.9 = " p999 " µs"
}'