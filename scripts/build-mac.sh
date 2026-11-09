#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ $(uname -s) == Darwin ]] || { echo 'Run this script on macOS.'; exit 1; }
xcrun --find clang++ >/dev/null
bundle="$PWD/build/GrossVideo.ofx.bundle"
mkdir -p "$bundle/Contents/MacOS"
xcrun clang++ -std=c++17 -O2 -Wall -Wextra -arch arm64 -arch x86_64 -mmacosx-version-min=11.0 -bundle -fvisibility=hidden -I vendor/openfx src/plugin.cpp -o "$bundle/Contents/MacOS/GrossVideo.ofx"
cat > "$bundle/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict><key>CFBundleIdentifier</key><string>com.tristokoni.grossvideo</string><key>CFBundleName</key><string>Gross Video</string><key>CFBundleExecutable</key><string>GrossVideo.ofx</string><key>CFBundlePackageType</key><string>BNDL</string><key>CFBundleVersion</key><string>1.0</string></dict></plist>
PLIST
codesign --force --sign - "$bundle"
codesign --verify --strict "$bundle"
echo "Built: $bundle"
