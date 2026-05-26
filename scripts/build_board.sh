#!/usr/bin/env bash
# Build helper: select board via esp_board_manager, then compile.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BOARD="${1:-esp_SensairShuttle}"
BOARD_PATH="${2:-boards/espressif}"

cd "$ROOT"

if [[ -f "$HOME/esp/esp-idf/export.sh" ]]; then
  # shellcheck disable=SC1091
  . "$HOME/esp/esp-idf/export.sh"
fi

echo "==> gen-bmgr-config: board=${BOARD}, path=${BOARD_PATH}"
idf.py gen-bmgr-config -c "${BOARD_PATH}" -b "${BOARD}"

echo "==> build"
idf.py build "$@"
