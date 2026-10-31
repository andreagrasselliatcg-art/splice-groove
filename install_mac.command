#!/bin/bash
# Installs Splice Groove (AU + VST3) for Logic Pro. Double-click this file in Finder.
set -e
cd "$(dirname "$0")"

AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "$AU_DIR" "$VST3_DIR"

if [ -d "Splice Groove.component" ]; then
  rm -rf "$AU_DIR/Splice Groove.component"
  cp -R "Splice Groove.component" "$AU_DIR/"
  xattr -dr com.apple.quarantine "$AU_DIR/Splice Groove.component" 2>/dev/null || true
  codesign --force --deep -s - "$AU_DIR/Splice Groove.component" >/dev/null 2>&1 || true
  echo "AU installed in $AU_DIR"
fi

if [ -d "Splice Groove.vst3" ]; then
  rm -rf "$VST3_DIR/Splice Groove.vst3"
  cp -R "Splice Groove.vst3" "$VST3_DIR/"
  xattr -dr com.apple.quarantine "$VST3_DIR/Splice Groove.vst3" 2>/dev/null || true
  codesign --force --deep -s - "$VST3_DIR/Splice Groove.vst3" >/dev/null 2>&1 || true
  echo "VST3 installed in $VST3_DIR"
fi

# Make Logic re-scan Audio Units
killall -9 AudioComponentRegistrar 2>/dev/null || true
echo ""
echo "Validating the Audio Unit..."
auval -v aumu SpGv Agrs | tail -3 || true
echo ""
echo "Done. Open Logic Pro: New Software Instrument track > AU Instruments > Andrea Grasselli > Splice Groove"
