#!/usr/bin/env bash
# Stress-tests miniredis with concurrent nc clients.
# Usage: ./scripts/stress.sh [clients] [ops_each]
# Requires: the server to already be running on localhost:6380

set -euo pipefail

CLIENTS=${1:-8}
OPS=${2:-1000}
PORT=6380
PASS=0
FAIL=0

run_client() {
    local id=$1
    for i in $(seq 1 "$OPS"); do
        local key="stress:${id}:${i}"
        # SET then GET; verify response
        set_resp=$(echo -e "SET $key val${i}\r" | nc -q1 localhost $PORT)
        get_resp=$(echo -e "GET $key\r"         | nc -q1 localhost $PORT)
        if [[ "$set_resp" != *"OK"* || "$get_resp" != *"val${i}"* ]]; then
            echo "FAIL client=$id key=$key"
            return 1
        fi
    done
    echo "OK  client=$id ops=$OPS"
}

export -f run_client
export OPS PORT

echo "Launching $CLIENTS concurrent clients, $OPS ops each..."
results=$(seq 1 "$CLIENTS" | xargs -P "$CLIENTS" -I{} bash -c 'run_client "$@"' _ {})

echo "$results"
PASS=$(echo "$results" | grep -c "^OK"  || true)
FAIL=$(echo "$results" | grep -c "^FAIL" || true)

echo ""
echo "Stress test complete: $PASS passed, $FAIL failed"
[[ $FAIL -eq 0 ]]