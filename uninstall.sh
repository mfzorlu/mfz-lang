#!/usr/bin/env bash
# uninstall.sh — MFZ Lang Linux Kaldırıcı
#
# Kullanım:
#   chmod +x uninstall.sh
#   ./uninstall.sh

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[*]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
removed() { echo -e "${RED}[-]${NC} $1"; }

echo ""
echo "  MFZ Lang Kaldırıcı"
echo "  ────────────────────────"
echo ""

# ── 1. Binary ──
info "Binary kaldırılıyor..."
if [ -f /usr/local/bin/mfz ]; then
    sudo rm -f /usr/local/bin/mfz
    removed "/usr/local/bin/mfz silindi."
else
    echo "    /usr/local/bin/mfz zaten yok."
fi

# ── 2. MIME tipi ──
info "MIME tipi kaldırılıyor..."
rm -f "$HOME/.local/share/mime/packages/mfz.xml"
rm -f "$HOME/.local/share/mime/text/x-mfz.xml"
update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true
removed "MIME tanımı silindi."

# ── 3. İkonlar ──
info "İkon dosyaları kaldırılıyor..."
SIZES="16 32 48 64 128 256"
for SIZE in $SIZES; do
    rm -f "$HOME/.local/share/icons/hicolor/${SIZE}x${SIZE}/mimetypes/text-x-mfz.png"
    rm -f "$HOME/.local/share/icons/Yaru/${SIZE}x${SIZE}/mimetypes/text-x-mfz.png"
done
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/Yaru"    2>/dev/null || true
removed "İkonlar silindi."

# ── 4. .desktop dosyası ──
info ".desktop dosyası kaldırılıyor..."
rm -f "$HOME/.local/share/applications/mfz.desktop"
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
removed ".desktop dosyası silindi."

# ── 5. İkon temasını yenile ──
CURRENT_THEME=$(gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d "'")
if [ -n "$CURRENT_THEME" ]; then
    gsettings set org.gnome.desktop.interface icon-theme 'hicolor'       2>/dev/null || true
    sleep 0.5
    gsettings set org.gnome.desktop.interface icon-theme "$CURRENT_THEME" 2>/dev/null || true
fi

echo ""
echo "  ────────────────────────"
echo -e "  ${GREEN}Kaldırma tamamlandı.${NC}"
echo "  ────────────────────────"
echo ""
echo "  Not: mfz-lang proje dizini silinmedi."
echo "  Tamamen silmek için: rm -rf <proje-dizini>"
echo ""
