#!/usr/bin/env bash
# Запуск части 2 (FIFO) одной командой.
set -euo pipefail

cd "$(dirname "$0")/.."

BIN=build
G2F=/tmp/gitstat_g2f.fifo
F2R=/tmp/gitstat_f2r.fifo

rm -f "$G2F" "$F2R"

# rep и flt блокируются на open() до появления второй стороны,
# поэтому порядок запуска не важен — главное запустить все три.
"$BIN/rep_gitstat" &
rep_pid=$!

"$BIN/flt_gitstat" &
flt_pid=$!

"$BIN/gen_gitstat" &
gen_pid=$!

status=0
wait "$gen_pid" || status=$?
wait "$flt_pid" || status=$?
wait "$rep_pid" || status=$?

exit "$status"
