#!/usr/bin/env bash
# install.sh — MFZ Lang system installer (Linux)
#
# Called by: make install
# Do NOT run directly unless you know what you're doing.
#
# What this does:
#   1. Copies mfz binary to /usr/local/bin/
#   2. Registers .mfz MIME type
#   3. Installs icon for all icon themes found on this system
#   4. Creates .desktop entry (double-click support)

set -e

RED='\033[0;31m'; GREEN='\033[0;32m'
YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

info()    { echo -e "${BLUE}[*]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
warning() { echo -e "${YELLOW}[!]${NC} $1"; }
error()   { echo -e "${RED}[✗]${NC} $1"; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="$SCRIPT_DIR/mfz/mfz"
ICON_PNG="$SCRIPT_DIR/mfz.png"
ICON_NAME="text-x-mfz"
MIME_TYPE="text/x-mfz"

echo ""
echo "  MFZ Lang — Installer"
echo "  ─────────────────────────"
echo ""

# ── Binary kontrolü ──
[ -f "$BINARY" ] || error "Binary not found: $BINARY — run 'make' first."

# ── 1. Binary kur ──
info "Installing binary to /usr/local/bin/mfz ..."
sudo install -m 755 "$BINARY" /usr/local/bin/mfz
success "mfz installed. You can now run 'mfz program.mfz' from anywhere."

# ── 2. MIME tipi ──
info "Registering .mfz MIME type ..."
mkdir -p "$HOME/.local/share/mime/packages"
cat > "$HOME/.local/share/mime/packages/mfz.xml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="text/x-mfz">
    <comment>MFZ Source File</comment>
    <glob pattern="*.mfz"/>
  </mime-type>
</mime-info>
EOF
update-mime-database "$HOME/.local/share/mime"
success "MIME type registered."

# ── 3. İkon ──
if [ -f "$ICON_PNG" ]; then
    info "Installing icons ..."

    # Her zaman hicolor'a koy (evrensel fallback)
    THEMES="hicolor"

    # Aktif ikon temasını ekle
    CURRENT_THEME=$(gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d "'" || echo "")
    [ -n "$CURRENT_THEME" ] && THEMES="$THEMES $CURRENT_THEME"

    # ~/.local/share/icons altındaki tüm kurulu temaları ekle
    if [ -d "$HOME/.local/share/icons" ]; then
        for d in "$HOME/.local/share/icons"/*/; do
            THEME_NAME=$(basename "$d")
            echo "$THEMES" | grep -qw "$THEME_NAME" || THEMES="$THEMES $THEME_NAME"
        done
    fi

    SIZES="16 32 48 64 128 256"
    HAS_CONVERT=0
    command -v convert >/dev/null 2>&1 && HAS_CONVERT=1

    for THEME in $THEMES; do
        for SIZE in $SIZES; do
            DIR="$HOME/.local/share/icons/$THEME/${SIZE}x${SIZE}/mimetypes"
            mkdir -p "$DIR"
            if [ "$HAS_CONVERT" -eq 1 ]; then
                convert "$ICON_PNG" -resize "${SIZE}x${SIZE}" "$DIR/${ICON_NAME}.png" 2>/dev/null || true
            else
                [ "$SIZE" = "256" ] && cp "$ICON_PNG" "$DIR/${ICON_NAME}.png"
            fi
        done
        gtk-update-icon-cache -f -t "$HOME/.local/share/icons/$THEME" 2>/dev/null || true
    done

    [ "$HAS_CONVERT" -eq 0 ] && warning "ImageMagick not found — only 256px icon installed. For best results: sudo apt install imagemagick"
    success "Icons installed for themes: $THEMES"
else
    warning "mfz.png not found — icon skipped."
fi

# ── 4. .desktop dosyası ──
info "Creating .desktop entry ..."
DESKTOP_DIR="$HOME/.local/share/applications"
mkdir -p "$DESKTOP_DIR"

if   command -v gnome-terminal >/dev/null 2>&1; then
    EXEC="gnome-terminal -- bash -c '/usr/local/bin/mfz %f; echo; read -p \"Press Enter to close...\"'"
elif command -v xterm >/dev/null 2>&1; then
    EXEC="xterm -e '/usr/local/bin/mfz %f; echo; read -p \"Press Enter...\"'"
elif command -v konsole >/dev/null 2>&1; then
    EXEC="konsole -e '/usr/local/bin/mfz %f'"
else
    EXEC="/usr/local/bin/mfz %f"
    warning "No terminal emulator found — double-click will run without a window."
fi

cat > "$DESKTOP_DIR/mfz.desktop" << EOF
[Desktop Entry]
Name=MFZ Interpreter
GenericName=MFZ Language Interpreter
Comment=Run .mfz source files
Exec=$EXEC
Icon=$ICON_NAME
Type=Application
MimeType=$MIME_TYPE;
Categories=Development;Utility;
Terminal=false
StartupNotify=false
EOF

chmod +x "$DESKTOP_DIR/mfz.desktop"
update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
xdg-mime default mfz.desktop "$MIME_TYPE" 2>/dev/null || true
success ".desktop entry created — .mfz files will open in terminal on double-click."

echo ""
echo "  ─────────────────────────"
echo -e "  ${GREEN}Installation complete!${NC}"
echo "  ─────────────────────────"
echo ""
echo "  Usage:"
echo "    mfz program.mfz"
echo "    mfz program.mfz --ast"
echo "    mfz program.mfz --tokens"
echo ""
echo "  To uninstall: make uninstall"
echo ""
echo "  Note: If the .mfz icon does not appear immediately,"
echo "  log out and back in, or run: nautilus -q"
echo ""
