#!/usr/bin/env bash
# install.sh — MFZ Lang Linux Yükleyici
#
# Kullanım:
#   chmod +x install.sh
#   ./install.sh
#
# Yapılanlar:
#   1. mfz binary'sini derler
#   2. /usr/local/bin/mfz'e kopyalar (PATH'e ekler)
#   3. .mfz MIME tipini sisteme tanıtır
#   4. İkonu sisteme kaydeder
#   5. Çift tıklayınca terminalde çalıştırmak için .desktop dosyası oluşturur

set -e  # Hata olursa dur

# ── Renkler ──
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[*]${NC} $1"; }
success() { echo -e "${GREEN}[✓]${NC} $1"; }
warning() { echo -e "${YELLOW}[!]${NC} $1"; }
error()   { echo -e "${RED}[✗]${NC} $1"; exit 1; }

# ── Çalışma dizini kontrolü ──
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MFZ_SRC="$SCRIPT_DIR/mfz"
ICON_FILE="$SCRIPT_DIR/mfz.png"

[ -f "$MFZ_SRC/main.c" ]        || error "mfz/ kaynak dizini bulunamadı. mfz-lang kök dizininden çalıştırın."
[ -f "$ICON_FILE" ]              || warning "mfz.png bulunamadı — ikon kurulmayacak."

echo ""
echo "  MFZ Lang Yükleyici"
echo "  ────────────────────────"
echo ""

# ── 1. Bağımlılık kontrolü ──
info "Bağımlılıklar kontrol ediliyor..."
command -v gcc  >/dev/null 2>&1 || error "gcc bulunamadı. Kurun: sudo apt install gcc"
command -v make >/dev/null 2>&1 || error "make bulunamadı. Kurun: sudo apt install make"
success "Bağımlılıklar tamam."

# ── 2. Derleme ──
info "Kaynak kod derleniyor..."
cd "$MFZ_SRC"
gcc -Wall -Wextra -std=c11 -o mfz lexer.c parser.c interpreter.c main.c -lm
cd "$SCRIPT_DIR"
success "Derleme tamamlandı."

# ── 3. Binary kurulumu ──
info "mfz binary'si /usr/local/bin/ dizinine kopyalanıyor..."
if sudo cp "$MFZ_SRC/mfz" /usr/local/bin/mfz; then
    sudo chmod +x /usr/local/bin/mfz
    success "mfz komutu sisteme eklendi. Artık her yerden 'mfz program.mfz' çalışır."
else
    warning "sudo başarısız — binary sadece proje dizininde kullanılabilir."
fi

# ── 4. MIME tipi tanımlama ──
info ".mfz MIME tipi sisteme tanıtılıyor..."
mkdir -p "$HOME/.local/share/mime/packages"
cat > "$HOME/.local/share/mime/packages/mfz.xml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
  <mime-type type="text/x-mfz">
    <comment>MFZ Source File</comment>
    <comment xml:lang="tr">MFZ Kaynak Dosyası</comment>
    <glob pattern="*.mfz"/>
    <magic priority="50">
      <match type="string" offset="0" value="//"/>
      <match type="string" offset="0" value="int "/>
      <match type="string" offset="0" value="float "/>
      <match type="string" offset="0" value="void "/>
    </magic>
  </mime-type>
</mime-info>
EOF
update-mime-database "$HOME/.local/share/mime"
success "MIME tipi kaydedildi."

# ── 5. İkon kurulumu ──
if [ -f "$ICON_FILE" ]; then
    info "İkon dosyaları kopyalanıyor..."

    # Yüklenecek boyutlar
    SIZES="16 32 48 64 128 256"
    ICON_NAME="text-x-mfz"

    # ImageMagick varsa çok boyutlu kur, yoksa sadece 256 koy
    if command -v convert >/dev/null 2>&1; then
        for SIZE in $SIZES; do
            DIR_HICOLOR="$HOME/.local/share/icons/hicolor/${SIZE}x${SIZE}/mimetypes"
            DIR_YARU="$HOME/.local/share/icons/Yaru/${SIZE}x${SIZE}/mimetypes"
            mkdir -p "$DIR_HICOLOR" "$DIR_YARU"
            convert "$ICON_FILE" -resize "${SIZE}x${SIZE}" "$DIR_HICOLOR/${ICON_NAME}.png" 2>/dev/null
            cp "$DIR_HICOLOR/${ICON_NAME}.png" "$DIR_YARU/${ICON_NAME}.png"
        done
        success "İkon ($SIZES px boyutlarında) kaydedildi."
    else
        DIR_HICOLOR="$HOME/.local/share/icons/hicolor/256x256/mimetypes"
        DIR_YARU="$HOME/.local/share/icons/Yaru/256x256/mimetypes"
        mkdir -p "$DIR_HICOLOR" "$DIR_YARU"
        cp "$ICON_FILE" "$DIR_HICOLOR/${ICON_NAME}.png"
        cp "$ICON_FILE" "$DIR_YARU/${ICON_NAME}.png"
        success "İkon (256px) kaydedildi. Daha iyi sonuç için: sudo apt install imagemagick"
    fi

    # Cache güncelle
    gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
    gtk-update-icon-cache -f -t "$HOME/.local/share/icons/Yaru"    2>/dev/null || true
else
    warning "İkon dosyası bulunamadı, atlanıyor."
fi

# ── 6. .desktop dosyası (çift tıklama desteği) ──
info ".desktop dosyası oluşturuluyor..."
DESKTOP_DIR="$HOME/.local/share/applications"
mkdir -p "$DESKTOP_DIR"

# Terminali bul
if   command -v gnome-terminal >/dev/null 2>&1; then TERM_CMD="gnome-terminal -- bash -c"
elif command -v xterm          >/dev/null 2>&1; then TERM_CMD="xterm -e bash -c"
elif command -v konsole        >/dev/null 2>&1; then TERM_CMD="konsole -e bash -c"
else                                                  TERM_CMD="bash -c"; warning "Terminal emülatörü bulunamadı."
fi

cat > "$DESKTOP_DIR/mfz.desktop" << EOF
[Desktop Entry]
Name=MFZ Interpreter
GenericName=MFZ Language Interpreter
Comment=Run MFZ source files
Exec=$TERM_CMD '/usr/local/bin/mfz %f; echo ""; echo "Çıkmak için Enter..."; read'
Icon=text-x-mfz
Type=Application
MimeType=text/x-mfz;
Categories=Development;Utility;
Terminal=false
StartupNotify=false
EOF

chmod +x "$DESKTOP_DIR/mfz.desktop"
update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
xdg-mime default mfz.desktop text/x-mfz 2>/dev/null || true
success ".desktop dosyası oluşturuldu. .mfz dosyalarına çift tıklayınca terminal açılır."

# ── 7. İkon yenileme tetikleyici ──
info "Masaüstü ortamı yenileniyor..."
CURRENT_THEME=$(gsettings get org.gnome.desktop.interface icon-theme 2>/dev/null | tr -d "'")
if [ -n "$CURRENT_THEME" ]; then
    gsettings set org.gnome.desktop.interface icon-theme 'hicolor'    2>/dev/null || true
    sleep 0.5
    gsettings set org.gnome.desktop.interface icon-theme "$CURRENT_THEME" 2>/dev/null || true
    success "İkon teması yenilendi."
fi

# ── Özet ──
echo ""
echo "  ────────────────────────"
echo -e "  ${GREEN}Kurulum tamamlandı!${NC}"
echo "  ────────────────────────"
echo ""
echo "  Kullanım:"
echo "    mfz program.mfz"
echo "    mfz program.mfz --ast"
echo "    mfz program.mfz --tokens"
echo ""
echo "  Kaldırmak için:"
echo "    ./uninstall.sh"
echo ""
