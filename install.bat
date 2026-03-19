@echo off
:: install.bat — MFZ Lang Windows Yükleyici
::
:: Kullanım:
::   Sağ tık → "Yönetici olarak çalıştır"
::
:: Yapılanlar:
::   1. mfz.exe binary'sini derler (gcc gerekli)
::   2. C:\Program Files\mfz\ dizinine kopyalar
::   3. PATH'e ekler
::   4. .mfz uzantısını kayıt defterine yazar (çift tıklama + ikon)

setlocal EnableDelayedExpansion
title MFZ Lang Yukleyici

echo.
echo   MFZ Lang Yukleyici
echo   ------------------------
echo.

:: ── Yönetici kontrolü ──
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] Bu dosyayi yonetici olarak calistirin.
    echo     Sag tik -^> "Yonetici olarak calistir"
    pause
    exit /b 1
)

:: ── Çalışma dizini ──
set "SCRIPT_DIR=%~dp0"
set "MFZ_SRC=%SCRIPT_DIR%mfz"
set "INSTALL_DIR=C:\Program Files\mfz"
set "ICON_FILE=%SCRIPT_DIR%mfz.ico"

if not exist "%MFZ_SRC%\main.c" (
    echo [x] mfz/ kaynak dizini bulunamadi.
    echo     mfz-lang kok dizininden calistirin.
    pause
    exit /b 1
)

:: ── 1. GCC kontrolü ──
echo [*] GCC kontrol ediliyor...
where gcc >nul 2>&1
if %errorLevel% neq 0 (
    echo [!] gcc bulunamadi.
    echo.
    echo     Secenekler:
    echo     1. MSYS2: https://www.msys2.org/
    echo        pacman -S mingw-w64-x86_64-gcc
    echo     2. WinLibs: https://winlibs.com/
    echo     3. TDM-GCC: https://jmeubank.github.io/tdm-gcc/
    echo.
    pause
    exit /b 1
)
echo [OK] GCC mevcut.

:: ── 2. Derleme ──
echo [*] Kaynak kod derleniyor...
cd /d "%MFZ_SRC%"
gcc -Wall -Wextra -std=c11 -o mfz.exe lexer.c parser.c interpreter.c main.c -lm
if %errorLevel% neq 0 (
    echo [x] Derleme basarisiz.
    pause
    exit /b 1
)
cd /d "%SCRIPT_DIR%"
echo [OK] Derleme tamamlandi.

:: ── 3. Kurulum dizini ──
echo [*] "%INSTALL_DIR%" dizinine kopyalaniyor...
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"
copy /y "%MFZ_SRC%\mfz.exe" "%INSTALL_DIR%\mfz.exe" >nul
echo [OK] mfz.exe kopyalandi.

:: ── 4. PATH'e ekle ──
echo [*] PATH guncelleniyor...
:: Mevcut PATH'i oku
for /f "tokens=2*" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "CURRENT_PATH=%%B"

:: Zaten ekli mi?
echo !CURRENT_PATH! | find /i "%INSTALL_DIR%" >nul
if %errorLevel% equ 0 (
    echo     PATH zaten guncelli.
) else (
    reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" ^
        /v Path /t REG_EXPAND_SZ /d "!CURRENT_PATH!;%INSTALL_DIR%" /f >nul
    echo [OK] PATH guncellendi. Yeni terminallerde 'mfz' komutu calisir.
)

:: ── 5. .mfz uzantısı — Kayıt Defteri ──
echo [*] .mfz dosya eslestirmesi kaydediliyor...

:: Dosya tipi tanımı
reg add "HKCR\.mfz"                           /ve /d "mfz.SourceFile"   /f >nul
reg add "HKCR\.mfz"                           /v "Content Type" /d "text/x-mfz" /f >nul
reg add "HKCR\mfz.SourceFile"                 /ve /d "MFZ Source File"  /f >nul
reg add "HKCR\mfz.SourceFile\DefaultIcon"     /ve /d "%INSTALL_DIR%\mfz.exe,0" /f >nul

:: Çift tıklama — terminalde aç
reg add "HKCR\mfz.SourceFile\shell\open\command" /ve /d "cmd.exe /k \"%INSTALL_DIR%\mfz.exe\" \"%%1\"" /f >nul

:: Sağ tık menüsü — "MFZ ile çalıştır"
reg add "HKCR\mfz.SourceFile\shell\run"              /ve /d "MFZ ile Calistir"   /f >nul
reg add "HKCR\mfz.SourceFile\shell\run\command"      /ve /d "cmd.exe /k \"%INSTALL_DIR%\mfz.exe\" \"%%1\"" /f >nul

reg add "HKCR\mfz.SourceFile\shell\run_ast"          /ve /d "AST Goster"        /f >nul
reg add "HKCR\mfz.SourceFile\shell\run_ast\command"  /ve /d "cmd.exe /k \"%INSTALL_DIR%\mfz.exe\" \"%%1\" --ast" /f >nul

reg add "HKCR\mfz.SourceFile\shell\run_tok"          /ve /d "Tokenlari Goster"  /f >nul
reg add "HKCR\mfz.SourceFile\shell\run_tok\command"  /ve /d "cmd.exe /k \"%INSTALL_DIR%\mfz.exe\" \"%%1\" --tokens" /f >nul

echo [OK] Dosya eslestirmesi kaydedildi.

:: ── 6. İkon (.ico varsa) ──
if exist "%ICON_FILE%" (
    copy /y "%ICON_FILE%" "%INSTALL_DIR%\mfz.ico" >nul
    reg add "HKCR\mfz.SourceFile\DefaultIcon" /ve /d "%INSTALL_DIR%\mfz.ico" /f >nul
    echo [OK] Ikon kaydedildi.
) else (
    echo [!] mfz.ico bulunamadi - ikon atlandi.
    echo     PNG'yi ICO'ya donusturun: https://convertio.co/png-ico/
    echo     Sonra install.bat'i tekrar calistirin.
)

:: ── 7. Explorer'ı yenile ──
echo [*] Dosya gezgini yenileniyor...
taskkill /f /im explorer.exe >nul 2>&1
start explorer.exe
echo [OK] Dosya gezgini yenilendi.

:: ── Özet ──
echo.
echo   ------------------------
echo   Kurulum tamamlandi!
echo   ------------------------
echo.
echo   Kullanim:
echo     mfz program.mfz
echo     mfz program.mfz --ast
echo     mfz program.mfz --tokens
echo.
echo   .mfz dosyalarina cift tiklayinca terminal acilir.
echo   Sag tik menusu: "MFZ ile Calistir" / "AST Goster"
echo.
echo   Kaldirmak icin: uninstall.bat (yonetici olarak)
echo.
pause
