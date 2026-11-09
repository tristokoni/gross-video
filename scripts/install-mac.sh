#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build-mac.sh
target='/Library/OFX/Plugins/GrossVideo.ofx.bundle'
if [[ -e "$target" ]]; then
 echo "Existing plugin found at $target. Move it aside before installing."
 exit 1
fi
sudo mkdir -p /Library/OFX/Plugins
sudo ditto build/GrossVideo.ofx.bundle "$target"
echo 'Installed. Restart DaVinci Resolve and search Effects for Gross Video.'
