# Nav++ — C++ WebView2 Browser

Nav++ is a real Windows desktop browser written in **C++ (Win32 + WebView2)**.
Borderless Chrome-like UI, multi-tabs, history / favorites / downloads / local accounts,
`custom.css` theming, 30 installer languages, DuckDuckGo search, and a real
**Inno Setup installer** (`Nav++-Setup-1.0.0.exe`).

> Repo: `https://github.com/Motatadev/nav-...` (complete the name, e.g. `nav-plus-plus`)
> Tutorial language: English. App + installer support 30 languages.

---

## 1. Features

- Borderless window (custom title bar, drag, resize, Windows `-` `□` `X` buttons)
- Multi-tabs (up to 8 WebView2 controllers, switch / `+` / `x`)
- Toolbar: back, forward, reload, address bar (Enter = Go), star, profile, menu
- Sidebar: home, history, favorites, downloads, settings
- Internal pages (generated HTML): new tab, history, favorites, downloads, profile/accounts, settings/themes
- Local accounts (`%APPDATA%\Nav++\accounts.txt`, current in `profile.txt`)
- History (`history.txt`), favorites (`favorites.txt`), downloads to `%USERPROFILE%\Downloads`
- Theming via `%APPDATA%\Nav++\custom.css` (`--bg --toolbar --text --accent --hover`) + presets
- Search: DuckDuckGo (address bar fallback + new-tab page)
- WebView2 runtime (Edge Chromium) — evergreen compatible

---

## 2. Requirements

| Tool | Version | Notes |
|---|---|---|
| Windows | 10 / 11 x64 | WebView2 needs Win10+ |
| Visual Studio | 2022+ or VS 18, **Desktop C++** workload | Provides MSVC `v145`, Windows SDK |
| WebView2 Runtime | Evergreen (e.g. 152+) | Preinstalled on most PCs; installer warns if missing |
| NuGet package | `Microsoft.Web.WebView2` 1.0.2903.40 | Restored via `packages.config` |
| Inno Setup 6 | 6.7+ | `ISCC.exe` builds the installer |
| ImageMagick 7 (optional) | 7.1+ | Only to rebuild `app.ico` from PNG |

Check MSVC present:

```powershell
Get-ChildItem "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC"
```

---

## 3. Project structure

```
nav-plus-plus/
  NavPlusPlus.sln
  installer.iss                 # Inno Setup script (30 langs, registry, shortcuts)
  README.md                     # this tutorial
  src/
    main.cpp                    # ~1000 lines: UI + tabs + backend + themes + langs
    newtab.html                 # start page (SVG icons, DDG search, ?lang=&theme=)
    app.rc                      # version info (Nav++ 1.0.0.0) + icon
    app.ico                     # multi-size icon (from logo.png via ImageMagick)
    logo.png                   # source logo (save yours here)
    packages.config             # Microsoft.Web.WebView2
    NavPlusPlus.vcxproj       # TargetName Nav++.exe, toolset v145
  x64/Release/Nav++.exe         # build output
  x64/Release/WebView2Loader.dll
  installer/Nav++-Setup-1.0.0.exe
```

Runtime data (per user, auto-created):

```
%APPDATA%\Nav++\
  history.txt  favorites.txt  accounts.txt  profile.txt
  history.html favorites.html downloads.html profile.html settings.html
  custom.css   debug.log      WebViewData\
```

---

## 4. Build

### 4.1 Restore WebView2 (first time)

```powershell
C:\Users\Motata\AppData\Local\Temp\opencode\nuget.exe restore "C:\Users\Motata\Downloads\nav-plus-plus\NavPlusPlus.sln"
```

Or: open `NavPlusPlus.sln` in Visual Studio → right-click solution → Restore NuGet.

### 4.2 Build from Visual Studio

1. Open `NavPlusPlus.sln` (VS 18 Community).
2. Select `x64` + `Release`.
3. `Build → Build Solution` (or `F5` to run).

### 4.3 Build from command line

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" `
  "C:\Users\Motata\Downloads\nav-plus-plus\NavPlusPlus.sln" `
  /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
```

Output: `x64\Release\Nav++.exe` (+ `WebView2Loader.dll` auto-copied by the NuGet targets).

### 4.4 Run

```powershell
Start-Process "C:\Users\Motata\Downloads\nav-plus-plus\x64\Release\Nav++.exe"
```

Close all copies before rebuilding, else `LNK1168`:

```powershell
Stop-Process -Name "Nav++" -Force -ErrorAction SilentlyContinue
```

---

## 5. Installer (real Windows app)

`installer.iss` produces a real installer: `C:\Program Files\Nav++`,
Start-Menu + Desktop shortcuts, Add/Remove Programs entry, uninstaller,
WebView2 check, 30-language selector, `HKLM/HKCU\Software\Nav++\Lang`.

```powershell
& "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe" `
  "C:\Users\Motata\Downloads\nav-plus-plus\installer.iss"
# → installer\Nav++-Setup-1.0.0.exe
```

> After changing language at install, the app reads it from the registry
> (`GetAppLang()` → HKCU then HKLM). Reinstall to switch.

---

## 6. Portable version (test without installing)

No installer files and no `Nav++.exe` are tracked in git — binaries stay local.
For quick retests, build the portable zip (never committed, see `.gitignore`):

```powershell
# 1. Build Release (§4), then pack:
New-Item -ItemType Directory -Path "$env:TEMP\Nav++-Portable" -Force
Copy-Item "C:\Users\Motata\Downloads\nav-plus-plus\x64\Release\Nav++.exe",
  "C:\Users\Motata\Downloads\nav-plus-plus\x64\Release\WebView2Loader.dll",
  "C:\Users\Motata\Downloads\nav-plus-plus\src\newtab.html" `
  -Destination "$env:TEMP\Nav++-Portable\" -Force
Compress-Archive -Path "$env:TEMP\Nav++-Portable\*" `
  -DestinationPath "C:\Users\Motata\Downloads\nav-plus-plus\installer\Nav++-Portable-1.0.0.zip" -Force
```

Unzip anywhere, keep the files together, double-click `Nav++.exe`.
Needs WebView2 Runtime (preinstalled on Win10/11).
Close every other Nav++ copy first — all copies share
`%APPDATA%\Nav++\WebViewData`, and WebView2 locks it to one instance.

---

## 7. Theming with `custom.css` (A–Z)

File: `%APPDATA%\Nav++\custom.css` (auto-created on first run).

```css
:root {
  --bg: #202124;
  --toolbar: #202124;
  --text: #9AA0A6;
  --accent: #8AB4F8;
  --hover: #3C4043;
}
/* your page rules below */
```

### 6.1 From the UI (no editor needed)

1. Click **gear** (sidebar) → **Settings** page.
2. Presets: **Chrome / Gaming / Clair / Bleu** → fills the editor.
3. Edit CSS in the textarea → **Appliquer** (writes file, `Theme_Load()`, repaints).
4. **Recharger** re-reads the file. **Ouvrir custom.css** opens Notepad.

### 6.2 How it works in code

- `ThemePath()`, `ThemeDefault()`, `ThemePreset(name)`, `Theme_Load()` parse
  `--var: #hex` with `HexColor()` into `g_theme` (`bg toolbar text accent hover`).
- Native UI uses `g_theme`: `DrawGamingBtn()`, `WM_PAINT`, `WM_CTLCOLORSTATIC/EDIT`.
- Internal pages inline it: `ShowInternalPage()` appends `<style>ThemeCssText()</style>`.
- `newtab.html` loads it via `?theme=file:///.../custom.css` link injection.
- `NavigationStarting` intercepts `file:///settings?action=save|preset|reload|open`.

### 6.3 Write your own theme

```css
:root {
  --bg: #0E0E1B;      /* top bar + sidebar */
  --toolbar: #12121F; /* active tab */
  --text: #C8A0FF;    /* icons */
  --accent: #A855F7;  /* active sidebar */
  --hover: #2D2A50;   /* hover fill */
}
a { border-radius: 12px !important; }
```

Save → Settings → **Recharger**. Broken file = fallback Chrome gray.

---

## 8. Languages (app follows the installer)

- Installer writes `{language}` to `HKLM/HKCU\Software\Nav++\Lang` (`[Registry]`).
- `GetAppLang()` + `T(key)` translate: `ph` (address placeholder),
  `new hist fav dl acc` (tabs + pages). English fallback for missing entries.
- `GetNewTabUrl()` appends `?lang=`; `newtab.html` JS swaps the placeholder.
- To add a language: add rows in `T()` (`main.cpp`), placeholder in `newtab.html`
  JS map, `.isl` in `installer.iss`.

---

## 9. How to code inside (recipes)

All in `src/main.cpp` unless noted.

### 8.1 Add a toolbar button

1. `const wchar_t* MdlIcon(int id)` → add `case 9: return L"\uE...";`
   (glyphs: `Segoe MDL2 Assets` cheat-sheet; no emoji).
2. `WM_CREATE` → `hBtnX = GamingBtn(hWnd, L"", x, y, w, h, 9);`
3. `WM_SIZE` → `MoveWindow(...)`; hover list in `WM_MOUSEMOVE`.
4. `WM_COMMAND` → `if (LOWORD(wParam) == 9) { ... }`.

### 8.2 Add a sidebar button + internal page

1. New ID (e.g. 25), icon in `MdlIcon()`, `GamingBtn()` in `WM_CREATE`.
2. New `kind` in `ShowInternalPage()` (build `html`, `WriteUtf8File`, `NavigateTo`).
3. Friendly name in `Tab_SetupEvents` → `add_SourceChanged`.
4. `WM_COMMAND`: `ShowInternalPage(L"mykind");`.

### 8.3 New tab behavior / search engine

- Start page: `GetNewTabUrl()` (+ `Tab_Create(hWnd, GetNewTabUrl())`).
- Address fallback: `WM_COMMAND` id `4` (`duckduckgo.com/?q=`).
- New-tab search: `newtab.html: go()`.

### 8.4 Accounts pattern (native ↔ page bridge)

Page links use fake URLs: `file:///profile?action=create&user=...`.
`add_NavigationStarting` parses, `put_Cancel(TRUE)`, acts (`Profile_SetUser`),
re-renders (`ShowInternalPage`). Same pattern for
`file:///settings?action=save&css=...`. Always `CoTaskMemFree(uri)`.

### 8.5 History / favorites

- `SourceChanged` → friendly tab titles + `History_Add(url, title)`
  (`file:///` excluded, `?` stripped from domains).
- Star button (id 6): `Favorites_Add()` (http only) → favorites page.

---

## 10. WebView2 gotchas (read before debugging)

| Symptom | Cause → fix |
|---|---|
| `0x800401F0` / env fail | COM not init → `CoInitializeEx` in `wWinMain` |
| Blank after Program-Files install | user-data dir not writable → `CreateCoreWebView2EnvironmentWithOptions(..., %APPDATA%\Nav++\WebViewData)` |
| `NavigateTo: webview null` | clicked before first tab ready → wait for `Environment OK` |
| `LNK1168` | exe running → kill `Nav++` before build |
| `RC2175` icon | PNG-only ICO → rebuild multi-size BMP ICO (ImageMagick, §10) |
| `?` in tabs | internal `file://` URLs → friendly names (§8.5) |
| `�` in pages | ANSI write → `WriteUtf8File` (BOM) + HTML entities |

---

## 11. Rename / version / icon

- Display name: `installer.iss` (`MyAppName`), `app.rc` (all `VALUE`s).
- Binary: `src/NavPlusPlus.vcxproj` → `<TargetName>Nav++</TargetName>`.
- Window: `wWinMain` class `NavPlusPlus`, title `Nav++`.
- Data dir: `GetDataDir()` (`%APPDATA%\Nav++`).
- Icon: save logo as `src\logo.png`, then

```powershell
& "$env:ProgramFiles\ImageMagick-7.1.2-Q16-HDRI\magick.exe" logo.png -define icon:auto-resize=16,24,32,48,64,128,256 app.ico
```

`app.rc` must contain `1 ICON "app.ico"`. Rebuild + recompile installer.

---

## 12. Publish to GitHub

```bash
cd C:/Users/Motata/Downloads/nav-plus-plus
git init
git add src/main.cpp src/newtab.html src/app.rc src/NavPlusPlus.vcxproj NavPlusPlus.sln installer.iss README.md .gitignore
git commit -m "Nav++ 1.0.0: tabs, themes, langs, installer"
git branch -M main
git remote add origin https://github.com/Motatadev/nav-plus-plus.git
git push -u origin main
```

> Do **not** commit `x64/`, `installer/*.exe`, `packages/`, `*.log`, `%APPDATA%` data.
> Add a `.gitignore` for them.

---

## 13. Roadmap ideas

- Real multi-window, pinned tabs, sessions restore
- Download manager UI (progress from `ICoreWebView2DownloadOperation`)
- Ad-block / extensions, devtools shortcut, print-to-PDF
- Auto-update (signed installer), per-language app UI beyond the 6 core keys
