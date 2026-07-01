#!/usr/bin/env bash
# Launch `tauri dev` from a shell that is NOT contaminated by the VSCode-snap
# environment. The snap exports GTK/GDK/GIO/GSETTINGS module paths pointing into
# /snap/code, whose modules are linked against core20's glibc — loading them into
# a system-built Tauri binary crashes with:
#   symbol lookup error: .../libpthread.so.0: undefined symbol __libc_pthread_init
# Stripping those vars makes the app use the system GTK stack. Run this from a
# normal (non-snap) terminal too — it's harmless there.
set -euo pipefail

# System toolchains (Node 20 via nvm, Rust via rustup).
export PATH="$HOME/.nvm/versions/node/v20.20.2/bin:$HOME/.cargo/bin:$PATH"

# Drop snap-injected GTK/GDK/GIO/locale module paths.
unset GTK_PATH GTK_EXE_PREFIX \
      GDK_PIXBUF_MODULEDIR GDK_PIXBUF_MODULE_FILE \
      GIO_MODULE_DIR GSETTINGS_SCHEMA_DIR GTK_IM_MODULE_FILE \
      LOCPATH SNAP_LIBRARY_PATH LD_LIBRARY_PATH LD_PRELOAD || true

# Point XDG at the system dirs so GSettings schemas resolve outside the snap.
export XDG_DATA_DIRS="/usr/local/share:/usr/share:/var/lib/snapd/desktop"
export XDG_CONFIG_DIRS="/etc/xdg/xdg-ubuntu:/etc/xdg"

cd "$(dirname "$0")"
exec npm run tauri dev
