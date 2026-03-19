#!/usr/bin/env bash
# uninstall.sh — MFZ Lang system uninstaller (Linux)
# Called by: make uninstall
# Works correctly both with and without sudo.

set -e

RED='\033[0;31m'; GREEN='\033[0;32m'; BLUE='\033[0;34m'; NC='\033[0m'
info()    { echo -e "${BLUE}[*]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
removed() { echo -e "${RED}[-]${NC} $1"; }

# ── Gerçek kullanıcıyı belirle ──
if [ -n "$SUDO_USER" ]; then
    REAL_USER="$SUDO_USER"
    REAL_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
else
    REAL_USER="$USER"
    REAL_HOME="$HOME"
fi

run_as_user() {
    if [ -n "$SUDO_USER" ]; then
        sudo -u "$SUDO_USER" "$@"
    else
        "$@"
    fi
}

echo ""
echo "  MFZ Lang — Uninstaller"
echo "  ─────────────────────────"
echo "  Removing for user: $REAL_USER"
echo ""

# 1. Binary
info "Removing binary ..."
if [ -f /usr/local/bin/mfz ]; then
    sudo rm -f /usr/local/bin/mfz
    removed "/usr/local/bin/mfz removed."
elif [ -f "$REAL_HOME/.local/bin/mfz" ]; then
    rm -f "$REAL_HOME/.local/bin/mfz"
    removed "~/.local/bin/mfz removed."
else
    echo "    Binary not found, skipping."
fi

# 2. MIME
info "Removing MIME type ..."
run_as_user rm -f "$REAL_HOME/.local/share/mime/packages/mfz.xml"
run_as_user rm -f "$REAL_HOME/.local/share/mime/text/x-mfz.xml"
run_as_user update-mime-database "$REAL_HOME/.local/share/mime" 2>/dev/null || true
removed "MIME type removed."

# 3. Icons — tüm temalarda ara ve sil
info "Removing icons ..."
SIZES="16 32 48 64 128 256"
if [ -d "$REAL_HOME/.local/share/icons" ]; then
    for THEME_DIR in "$REAL_HOME/.local/share/icons"/*/; do
        [ -d "$THEME_DIR" ] || continue
        for SIZE in $SIZES; do
            run_as_user rm -f "$THEME_DIR${SIZE}x${SIZE}/mimetypes/text-x-mfz.png"
        done
        run_as_user gtk-update-icon-cache -f -t "$THEME_DIR" 2>/dev/null || true
    done
fi
removed "Icons removed."

# 4. .desktop
info "Removing .desktop entry ..."
run_as_user rm -f "$REAL_HOME/.local/share/applications/mfz.desktop"
run_as_user update-desktop-database "$REAL_HOME/.local/share/applications" 2>/dev/null || true
removed ".desktop entry removed."

echo ""
echo "  ─────────────────────────"
echo -e "  ${GREEN}Uninstall complete.${NC}"
echo "  ─────────────────────────"
echo ""
echo "  Note: The mfz-lang project directory was not removed."
echo "  To fully remove it: rm -rf <project-dir>"
echo ""
