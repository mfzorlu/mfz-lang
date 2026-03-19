#!/usr/bin/env bash
# uninstall.sh — MFZ Lang system uninstaller (Linux)
# Called by: make uninstall

set -e

RED='\033[0;31m'; GREEN='\033[0;32m'; BLUE='\033[0;34m'; NC='\033[0m'
info()    { echo -e "${BLUE}[*]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
removed() { echo -e "${RED}[-]${NC} $1"; }

echo ""
echo "  MFZ Lang — Uninstaller"
echo "  ─────────────────────────"
echo ""

# 1. Binary
info "Removing binary ..."
if [ -f /usr/local/bin/mfz ]; then
    sudo rm -f /usr/local/bin/mfz
    removed "/usr/local/bin/mfz removed."
else
    echo "    /usr/local/bin/mfz not found, skipping."
fi

# 2. MIME
info "Removing MIME type ..."
rm -f "$HOME/.local/share/mime/packages/mfz.xml"
rm -f "$HOME/.local/share/mime/text/x-mfz.xml"
update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true
removed "MIME type removed."

# 3. Icons — tüm temalarda ara ve sil
info "Removing icons ..."
SIZES="16 32 48 64 128 256"
if [ -d "$HOME/.local/share/icons" ]; then
    for THEME_DIR in "$HOME/.local/share/icons"/*/; do
        for SIZE in $SIZES; do
            TARGET="$THEME_DIR${SIZE}x${SIZE}/mimetypes/text-x-mfz.png"
            rm -f "$TARGET"
        done
        gtk-update-icon-cache -f -t "$THEME_DIR" 2>/dev/null || true
    done
fi
removed "Icons removed."

# 4. .desktop
info "Removing .desktop entry ..."
rm -f "$HOME/.local/share/applications/mfz.desktop"
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
removed ".desktop entry removed."

echo ""
echo "  ─────────────────────────"
echo -e "  ${GREEN}Uninstall complete.${NC}"
echo "  ─────────────────────────"
echo ""
echo "  Note: The mfz-lang project directory was not removed."
echo "  To fully remove it: rm -rf <project-dir>"
echo ""
