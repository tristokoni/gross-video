#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build-mac.sh
target='/Library/OFX/Plugins/GrossVideo.ofx.bundle'
if [[ -e "$target" ]]; then
 backup="/Library/OFX/GrossVideo-backups/$(date +%Y%m%d-%H%M%S)-$$"
 sudo mkdir -p "$backup"
 sudo mv "$target" "$backup/"
 echo "Previous plugin backed up to $backup"
fi
sudo mkdir -p /Library/OFX/Plugins
sudo ditto build/GrossVideo.ofx.bundle "$target"
echo 'Installed. Restart DaVinci Resolve and search Effects for Gross Video.'
