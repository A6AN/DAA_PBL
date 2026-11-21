#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
make
printf '\n===== SCENARIO 1: NORMAL NEWSROOM =====\n'
./build/signaldesk --scenario 1
printf '\n===== SCENARIO 2: CAPACITY SHOCK =====\n'
./build/signaldesk --scenario 2
printf '\n===== SCENARIO 3: STRESS COMPARISON =====\n'
./build/signaldesk --scenario 3
printf '\n===== TESTS =====\n'
make test
