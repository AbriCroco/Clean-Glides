#!/usr/bin/env bash

# Packages a Clean Glides VST3 build for Linux or Windows.
#
# What it does:
#   1. merges the architecture builds into a single VST3 bundle
#   2. adds the install script, the Licenses folder and a README
#   3. zips everything

set -euo pipefail

if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <linux|windows> <artifacts-dir> <output-dir>" >&2
  exit 2
fi

OS_KEY="$1"
IN="$2"
OUT="$3"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$OS_KEY" in
  linux)   OS_LABEL="Linux";   BASE_ARCH="x86_64" ;;
  windows) OS_LABEL="Windows"; BASE_ARCH="x64" ;;
  *) echo "Unknown OS '$OS_KEY' (expected linux or windows)" >&2; exit 2 ;;
esac

[ -f CMakeLists.txt ] || { echo "Run this script from the repository root" >&2; exit 2; }
[ -d "$IN" ] || { echo "::error::Artifacts directory '$IN' not found"; exit 1; }
IN="$(cd "$IN" && pwd)"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

PKG="CleanGlides_VST3_${OS_LABEL}"
STAGE_ROOT="$(mktemp -d)"
LICENSE_SOURCE="$(mktemp -d)"
trap 'rm -rf "$STAGE_ROOT" "$LICENSE_SOURCE"' EXIT
STAGE="$STAGE_ROOT/$PKG"
mkdir -p "$STAGE"

VERSION=$(sed -nE 's/^project\(CleanGlides[[:space:]]+VERSION[[:space:]]+([0-9]+(\.[0-9]+)*).*/\1/p' CMakeLists.txt | head -n 1)
[ -n "$VERSION" ] || { echo "::error::Could not read the version from CMakeLists.txt"; exit 1; }
echo "Version: $VERSION"

# --- 1. Merge architectures into one bundle --------------------------------
BASE_DIR="$IN/vst3-${OS_KEY}-${BASE_ARCH}"
BASE_BUNDLE=$(find "$BASE_DIR" -maxdepth 1 -type d -name "*.vst3" 2>/dev/null | head -n 1 || true)
[ -n "$BASE_BUNDLE" ] || { echo "::error::Base bundle not found in $BASE_DIR"; exit 1; }
NAME="$(basename "$BASE_BUNDLE")"

cp -R "$BASE_BUNDLE" "$STAGE/$NAME"

# Copy only architecture folders that are not present yet; shared files
# (Resources, ...) stay as they are in the base build.
for dir in "$IN"/vst3-"${OS_KEY}"-*/; do
  dir="${dir%/}"
  [ "$dir" = "$BASE_DIR" ] && continue
  other=$(find "$dir" -maxdepth 1 -type d -name "*.vst3" | head -n 1 || true)
  [ -n "$other" ] || continue
  for arch_dir in "$other"/Contents/*/; do
    arch_name="$(basename "$arch_dir")"
    if [ ! -e "$STAGE/$NAME/Contents/$arch_name" ]; then
      cp -R "${arch_dir%/}" "$STAGE/$NAME/Contents/$arch_name"
    fi
  done
done

find "$STAGE" -type f \( -name "*.pdb" -o -name "*.exp" -o -name "*.lib" \) -delete
find "$STAGE" -type f -name "*.so" -exec chmod 755 {} +

# --- Architectures actually present (used in the README) -------------------
PRESENT=$(find "$STAGE/$NAME/Contents" -mindepth 1 -maxdepth 1 -type d ! -name Resources -printf '%f\n' | sort)
[ -n "$PRESENT" ] || { echo "::error::No architecture folder found in the bundle"; exit 1; }

friendly() {
  case "$1" in
    x86_64-linux)  echo "x86_64" ;;
    aarch64-linux) echo "aarch64" ;;
    x86_64-win)    echo "x64" ;;
    arm64-win)     echo "ARM64" ;;
    arm64ec-win)   echo "ARM64EC" ;;
    *)             echo "$1" ;;
  esac
}

ARCH_NAMES=()
for d in x86_64-linux aarch64-linux x86_64-win arm64-win arm64ec-win; do
  if grep -qx "$d" <<<"$PRESENT"; then ARCH_NAMES+=("$(friendly "$d")"); fi
done
while IFS= read -r d; do
  case "$d" in
    x86_64-linux|aarch64-linux|x86_64-win|arm64-win|arm64ec-win) ;;
    *) ARCH_NAMES+=("$d") ;;
  esac
done <<<"$PRESENT"

N="${#ARCH_NAMES[@]}"
ARCHS=""
for i in "${!ARCH_NAMES[@]}"; do
  if [ "$i" -eq 0 ]; then
    ARCHS="${ARCH_NAMES[$i]}"
  elif [ "$i" -eq $((N - 1)) ]; then
    ARCHS="$ARCHS and ${ARCH_NAMES[$i]}"
  else
    ARCHS="$ARCHS, ${ARCH_NAMES[$i]}"
  fi
done
echo "Architectures in the bundle: $ARCHS"

# --- 2a. Install script -----------------------------------------------------
case "$OS_KEY" in
  linux)
    cp "$HERE/linux/install.sh" "$STAGE/install.sh"
    chmod 755 "$STAGE/install.sh"
    ;;
  windows)
    cp "$HERE/windows/Install.bat" "$STAGE/Install.bat"
    ;;
esac

# --- 2b. README = installation part + licensing part ------------------------
sed "s/@ARCHS@/$ARCHS/g" "$HERE/readme/installation-$OS_KEY.txt" > "$STAGE/README.txt"
echo "" >> "$STAGE/README.txt"
sed -e "s/@VERSION@/$VERSION/g" -e "s/@SECTION@/2/g" -e "s/@CONTAINER@/archive/g" \
  "$HERE/readme/licensing.txt" >> "$STAGE/README.txt"

if grep -qE '@[A-Z]+@' "$STAGE/README.txt"; then
  echo "::error::Unreplaced placeholder left in README.txt"
  exit 1
fi

# --- 2c. Licenses folder ----------------------------------------------------
git fetch --no-tags --depth=1 origin main
git archive FETCH_HEAD assets/fonts/licenses | tar -x -C "$LICENSE_SOURCE"

test -f LICENSE || { echo "LICENSE not found"; exit 1; }
test -f THIRD_PARTY_LICENSES.md || { echo "THIRD_PARTY_LICENSES.md not found"; exit 1; }
test -d "$LICENSE_SOURCE/assets/fonts/licenses" || { echo "Font licenses directory not found"; exit 1; }

mkdir -p "$STAGE/Licenses"

cp LICENSE "$STAGE/Licenses/LICENSE.txt"

# THIRD_PARTY_LICENSES.md to plain text:
#   - strip <h1>..<h6> tags
#   - strip ** bold markers
#   - [text](url) -> text (url)
perl -pe 's{</?h[1-6]>}{}g; s/\*\*//g; s/\[([^\]]+)\]\(([^)]+)\)/$1 ($2)/g' \
  THIRD_PARTY_LICENSES.md > "$STAGE/Licenses/THIRD_PARTY_LICENSES.txt"

if grep -qE '</?h[1-6]>|\*\*|\]\(' "$STAGE/Licenses/THIRD_PARTY_LICENSES.txt"; then
  echo "::error::Markdown/HTML residue left in THIRD_PARTY_LICENSES.txt"
  exit 1
fi

while IFS= read -r -d '' file; do
  filename="$(basename "$file")"
  destination="$STAGE/Licenses/$filename"

  if [ -e "$destination" ]; then
    prefix="$(basename "$(dirname "$file")")"
    destination="$STAGE/Licenses/${prefix}-${filename}"
  fi

  cp "$file" "$destination"
done < <(find "$LICENSE_SOURCE/assets/fonts/licenses" -type f -print0)

FONT_LICENSE_COUNT=$(find "$LICENSE_SOURCE/assets/fonts/licenses" -type f | wc -l | tr -d ' ')
COPIED_FONT_LICENSE_COUNT=$(find "$STAGE/Licenses" -maxdepth 1 -type f \
  ! -name "LICENSE.txt" ! -name "THIRD_PARTY_LICENSES.txt" | wc -l | tr -d ' ')

if [ "$FONT_LICENSE_COUNT" -ne "$COPIED_FONT_LICENSE_COUNT" ]; then
  echo "::error::Font license count mismatch!"
  echo "Expected: $FONT_LICENSE_COUNT"
  echo "Copied: $COPIED_FONT_LICENSE_COUNT"
  exit 1
fi

echo "All licenses successfully included:"
echo "AGPLv3: OK"
echo "Third-party licenses: OK"
echo "Font licenses: $COPIED_FONT_LICENSE_COUNT"

# --- 3. Windows text files use CRLF ----------------------------------------
if [ "$OS_KEY" = "windows" ]; then
  sed -i 's/$/\r/' "$STAGE/README.txt" "$STAGE/Install.bat"
fi

# --- 4. Zip -----------------------------------------------------------------
ZIP_NAME="Clean Glides_VST3_${OS_LABEL}.zip"
rm -f "$OUT/$ZIP_NAME"
(cd "$STAGE_ROOT" && zip -r -q -y "$OUT/$ZIP_NAME" "$PKG")

echo "Package contents:"
unzip -l "$OUT/$ZIP_NAME"

if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
  {
    echo "### $ZIP_NAME"
    echo "- Version: $VERSION"
    echo "- Architectures: $ARCHS"
  } >> "$GITHUB_STEP_SUMMARY"
fi