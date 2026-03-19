@echo off
:: uninstall.bat — MFZ Lang Windows Kaldırıcı
::
:: Kullanım:
::   Sağ tık → "Yönetici olarak çalıştır"

setlocal
title MFZ Lang Kaldirici

echo.
echo   MFZ Lang Kaldirici
echo   ------------------------
echo.

:: ── Yönetici kontrolü ──
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] Bu dosyayi yonetici olarak calistirin.
    pause
    exit /b 1
)

set "INSTALL_DIR=C:\Program Files\mfz"

:: ── 1. Dosyaları sil ──
echo [*] Program dosyalari kaldiriliyor...
if exist "%INSTALL_DIR%" (
    rd /s /q "%INSTALL_DIR%"
    echo [-] "%INSTALL_DIR%" silindi.
) else (
    echo     "%INSTALL_DIR%" zaten yok.
)

:: ── 2. PATH'den çıkar ──
echo [*] PATH guncelleniyor...
for /f "tokens=2*" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "CURRENT_PATH=%%B"
set "NEW_PATH=!CURRENT_PATH:%INSTALL_DIR%;=!"
set "NEW_PATH=!NEW_PATH:;%INSTALL_DIR%=!"
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" ^
    /v Path /t REG_EXPAND_SZ /d "!NEW_PATH!" /f >nul
echo [-] PATH guncellendi.

:: ── 3. Kayıt defteri girdilerini sil ──
echo [*] Kayit defteri girdileri kaldiriliyor...
reg delete "HKCR\.mfz"           /f >nul 2>&1
reg delete "HKCR\mfz.SourceFile" /f >nul 2>&1
echo [-] Kayit defteri girdileri silindi.

:: ── 4. Explorer'ı yenile ──
taskkill /f /im explorer.exe >nul 2>&1
start explorer.exe

echo.
echo   ------------------------
echo   Kaldirma tamamlandi.
echo   ------------------------
echo.
echo   Not: mfz-lang proje dizini silinmedi.
echo   Tamamen silmek icin proje klasorunu manuel silin.
echo.
pause
