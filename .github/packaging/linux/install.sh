#!/usr/bin/env bash
set -euo pipefail

NAME="Clean Glides.vst3"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  echo "Usage: ./install.sh [--user | --system] [--all]"
  echo "  --user    install to ~/.vst3 (default, no sudo needed)"
  echo "  --system  install to /usr/local/lib/vst3 for all users (uses sudo)"
  echo "  --all     keep every architecture build (default: only the one"
  echo "            matching this machine)"
}

DEST="$HOME/.vst3"
SUDO=""
ALL=0

for arg in "$@"; do
  case "$arg" in
    --user)     DEST="$HOME/.vst3"; SUDO="" ;;
    --system)   DEST="/usr/local/lib/vst3"; SUDO="sudo" ;;
    --all)      ALL=1 ;;
    -h|--help)  usage; exit 0 ;;
    *)          usage; exit 1 ;;
  esac
done

if [ ! -d "$HERE/$NAME" ]; then
  echo "Error: \"$NAME\" was not found next to this script." >&2
  echo "Extract the whole ZIP archive first, then run ./install.sh again." >&2
  exit 1
fi

KEEP=""
DROP=""
case "$(uname -m)" in
  x86_64|amd64)  KEEP="x86_64-linux";  DROP="aarch64-linux" ;;
  aarch64|arm64) KEEP="aarch64-linux"; DROP="x86_64-linux" ;;
esac

echo "Installing to: $DEST/$NAME"
$SUDO mkdir -p "$DEST"
$SUDO rm -rf "${DEST:?}/$NAME"
$SUDO cp -R "$HERE/$NAME" "$DEST/$NAME"

# Remove the build this machine cannot use, but only if the build it
# needs is really there.
if [ "$ALL" -eq 0 ] && [ -n "$DROP" ] && [ -d "$HERE/$NAME/Contents/$KEEP" ]; then
  $SUDO rm -rf "${DEST:?}/$NAME/Contents/$DROP"
  echo "Installed the $KEEP build only (use --all to keep every build)."
else
  echo "Installed every build included in the archive."
fi

echo
echo "Clean Glides was installed successfully."
echo "Restart your DAW and rescan your plugins if needed."