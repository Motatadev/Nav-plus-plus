#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <shlobj.h>
#include <dwmapi.h>
#include <wrl.h>
#include <WebView2.h>
#pragma comment(lib, "dwmapi.lib")

using namespace Microsoft::WRL;

HWND hUrlBar, hWebViewParent;
ComPtr<ICoreWebView2> webview; // = onglet actif (alias)
ComPtr<ICoreWebView2Controller> controller; // = onglet actif
ComPtr<ICoreWebView2Environment> g_env;
HWND g_mainWnd = nullptr;
WNDPROC g_oldEdit = nullptr;

struct Tab {
    ComPtr<ICoreWebView2Controller> ctl;
    ComPtr<ICoreWebView2> wv;
    std::wstring title = L"New tab";
};
std::vector<Tab> g_tabs;
int g_active = -1;
HWND hTabBtns[8] = {}, hTabX[8] = {};
HWND g_tabPlus = nullptr;
void Tab_Refresh(HWND hWnd);
void Tab_Switch(HWND hWnd, int i);
void Tab_Create(HWND hWnd, const std::wstring& url);
void Tab_Close(HWND hWnd, int i);

LRESULT CALLBACK EditSubclass(HWND hEdit, UINT m, WPARAM wp, LPARAM lp) {
    if (m == WM_KEYDOWN && wp == VK_RETURN) {
        SendMessageW(GetParent(hEdit), WM_COMMAND, 4, 0);
        return 0;
    }
    return CallWindowProcW(g_oldEdit, hEdit, m, wp, lp);
}

// ===== Langue choisie a l'install (HKCU\Software\Nav++\Lang) =====
std::wstring GetAppLang() {
    wchar_t v[64] = {};
    DWORD n = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Nav++", L"Lang", RRF_RT_REG_SZ, nullptr, v, &n) == ERROR_SUCCESS && v[0])
        return v;
    n = sizeof(v); v[0] = 0;
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"Software\\Nav++", L"Lang", RRF_RT_REG_SZ, nullptr, v, &n) == ERROR_SUCCESS && v[0])
        return v;
    return L"french";
}
std::wstring T(const std::wstring& key) {
    // table 30 langues setup -> FR par defaut, EN fallback
    static std::wstring lang;
    if (lang.empty()) lang = GetAppLang();
    struct E { const wchar_t* l; const wchar_t* k; const wchar_t* v; };
    static const E d[] = {
        // placeholder barre d'adresse
        {L"french",L"ph",L"Rechercher ou saisir une adresse"},{L"english",L"ph",L"Search or enter address"},
        {L"spanish",L"ph",L"Buscar o escribir una direccion"},{L"german",L"ph",L"Suchen oder Adresse eingeben"},
        {L"italian",L"ph",L"Cerca o inserisci un indirizzo"},{L"portuguese",L"ph",L"Pesquisar ou inserir endereco"},
        {L"brazilianportuguese",L"ph",L"Pesquisar ou inserir endereco"},{L"russian",L"ph",L"Poisk ili vvedite adres"},
        {L"arabic",L"ph",L"Abhath aw adkhal onwan"},{L"dutch",L"ph",L"Zoeken of adres invoeren"},
        {L"polish",L"ph",L"Szukaj lub wpisz adres"},{L"turkish",L"ph",L"Ara veya adres gir"},
        {L"japanese",L"ph",L"Kensaku mataha adoresu wo nyuryoku"},{L"korean",L"ph",L"Geomsaek ttoneun juso iblyeok"},
        {L"ukrainian",L"ph",L"Poshuk abo vvedit adresu"},
        // onglets
        {L"french",L"new",L"Nouvel onglet"},{L"english",L"new",L"New tab"},
        {L"spanish",L"new",L"Nueva pestana"},{L"german",L"new",L"Neuer Tab"},
        {L"italian",L"new",L"Nuova scheda"},{L"portuguese",L"new",L"Novo separador"},
        {L"brazilianportuguese",L"new",L"Nova guia"},{L"russian",L"new",L"Novaya vkladka"},
        {L"arabic",L"new",L"Tab jadid"},{L"dutch",L"new",L"Nieuw tabblad"},
        {L"polish",L"new",L"Nowa karta"},{L"turkish",L"new",L"Yeni sekme"},
        {L"japanese",L"new",L"Atarashii tabu"},{L"korean",L"new",L"Sae taeb"},
        {L"ukrainian",L"new",L"Nova vkladka"},
        {L"french",L"hist",L"Historique"},{L"english",L"hist",L"History"},
        {L"spanish",L"hist",L"Historial"},{L"german",L"hist",L"Verlauf"},
        {L"italian",L"hist",L"Cronologia"},{L"portuguese",L"hist",L"Historico"},
        {L"brazilianportuguese",L"hist",L"Historico"},{L"russian",L"hist",L"Istoria"},
        {L"arabic",L"hist",L"Sijil"},{L"dutch",L"hist",L"Geschiedenis"},
        {L"polish",L"hist",L"Historia"},{L"turkish",L"hist",L"Gecmis"},
        {L"japanese",L"hist",L"Rireki"},{L"korean",L"hist",L"Hijoseori"},
        {L"ukrainian",L"hist",L"Istoria"},
        {L"french",L"fav",L"Favoris"},{L"english",L"fav",L"Favorites"},
        {L"spanish",L"fav",L"Favoritos"},{L"german",L"fav",L"Favoriten"},
        {L"italian",L"fav",L"Preferiti"},{L"portuguese",L"fav",L"Favoritos"},
        {L"brazilianportuguese",L"fav",L"Favoritos"},{L"russian",L"fav",L"Izbrannoe"},
        {L"arabic",L"fav",L"Mofaddala"},{L"dutch",L"fav",L"Favorieten"},
        {L"polish",L"fav",L"Ulubione"},{L"turkish",L"fav",L"Sik kullanilanlar"},
        {L"japanese",L"fav",L"Okiniiri"},{L"korean",L"fav",L"Jeulgyeochatki"},
        {L"ukrainian",L"fav",L"Vybrane"},
        {L"french",L"dl",L"Telechargements"},{L"english",L"dl",L"Downloads"},
        {L"spanish",L"dl",L"Descargas"},{L"german",L"dl",L"Downloads"},
        {L"italian",L"dl",L"Download"},{L"portuguese",L"dl",L"Transferencias"},
        {L"brazilianportuguese",L"dl",L"Downloads"},{L"russian",L"dl",L"Zagruzki"},
        {L"arabic",L"dl",L"Tanzilat"},{L"dutch",L"dl",L"Downloads"},
        {L"polish",L"dl",L"Pobrane"},{L"turkish",L"dl",L"Indirilenler"},
        {L"japanese",L"dl",L"Daunrodo"},{L"korean",L"dl",L"Daunseu"},
        {L"ukrainian",L"dl",L"Zavantazhennya"},
        {L"french",L"acc",L"Compte"},{L"english",L"acc",L"Account"},
        {L"spanish",L"acc",L"Cuenta"},{L"german",L"acc",L"Konto"},
        {L"italian",L"acc",L"Account"},{L"portuguese",L"acc",L"Conta"},
        {L"brazilianportuguese",L"acc",L"Conta"},{L"russian",L"acc",L"Akkaunt"},
        {L"arabic",L"acc",L"Hisab"},{L"dutch",L"acc",L"Account"},
        {L"polish",L"acc",L"Konto"},{L"turkish",L"acc",L"Hesap"},
        {L"japanese",L"acc",L"Akaunto"},{L"korean",L"acc",L"Gyjeong"},
        {L"ukrainian",L"acc",L"Oblikovy zapys"},
    };
    for (auto &e : d) if (lang == e.l && key == e.k) return e.v;
    for (auto &e : d) if (std::wstring(L"english") == e.l && key == e.k) return e.v;
    return key;
}

// ===== Themes custom.css (couleurs GUI + pages) =====
std::wstring GetDataDir();
std::string ToUtf8(const std::wstring& w);
void WriteUtf8File(const std::wstring& path, const std::wstring& content);
struct Theme { COLORREF bg = RGB(32,33,36); COLORREF toolbar = RGB(32,33,36); COLORREF text = RGB(154,160,166); COLORREF accent = RGB(138,180,248); COLORREF hover = RGB(60,60,65); };
static Theme g_theme;
static HBRUSH g_bgBrush = nullptr, g_barBrush = nullptr;
static std::wstring ThemePath() { return GetDataDir() + L"\\custom.css"; }
static COLORREF HexColor(const std::wstring& h, COLORREF fb) {
    if (h.size() == 7 && h[0] == L'#') {
        int r=0,g=0,b=0;
        swscanf_s(h.c_str(), L"#%02x%02x%02x", &r, &g, &b);
        return RGB(r,g,b);
    }
    return fb;
}
static std::wstring ThemeDefault() {
    return L":root {\n  --bg: #202124;\n  --toolbar: #202124;\n  --text: #9AA0A6;\n  --accent: #8AB4F8;\n  --hover: #3C4043;\n}\n/* Mets tes regles pages ici. Change les --couleurs, Parametres > Recharger. */\n";
}
static std::wstring ThemePreset(const std::wstring& n) {
    if (n == L"gaming") return L":root {\n  --bg: #0E0E1B;\n  --toolbar: #12121F;\n  --text: #C8A0FF;\n  --accent: #A855F7;\n  --hover: #2D2A50;\n}\n";
    if (n == L"light") return L":root {\n  --bg: #FFFFFF;\n  --toolbar: #F1F3F4;\n  --text: #5F6368;\n  --accent: #1A73E8;\n  --hover: #E8EAED;\n}\n";
    if (n == L"blue") return L":root {\n  --bg: #0B1E3A;\n  --toolbar: #10294F;\n  --text: #8AB4F8;\n  --accent: #4285F4;\n  --hover: #1A3A5F;\n}\n";
    return ThemeDefault(); // chrome
}
static void Theme_Load() {
    std::wstring p = ThemePath();
    std::wifstream f(p);
    if (!f) { WriteUtf8File(p, ThemeDefault()); }
    std::wstring css, l;
    // lit en utf8->wide approx (ascii suffit pour couleurs)
    char buf[4096]; std::string all;
    std::ifstream bf(ToUtf8(p), std::ios::binary);
    if (bf) { all.assign((std::istreambuf_iterator<char>(bf)), std::istreambuf_iterator<char>()); }
    auto get = [&](const char* n, COLORREF fb) {
        std::string k = std::string(n) + ":";
        auto pos = all.find(k);
        if (pos == std::string::npos) return fb;
        auto h = all.find('#', pos);
        if (h == std::string::npos) return fb;
        std::string hex = all.substr(h, 7);
        std::wstring w(hex.begin(), hex.end());
        return HexColor(w, fb);
    };
    g_theme.bg = get("--bg", RGB(32,33,36));
    g_theme.toolbar = get("--toolbar", RGB(32,33,36));
    g_theme.text = get("--text", RGB(154,160,166));
    g_theme.accent = get("--accent", RGB(138,180,248));
    g_theme.hover = get("--hover", RGB(60,60,65));
    if (g_bgBrush) DeleteObject(g_bgBrush);
    if (g_barBrush) DeleteObject(g_barBrush);
    g_bgBrush = CreateSolidBrush(g_theme.bg);
    g_barBrush = CreateSolidBrush(g_theme.bg);
}
static std::wstring ThemeCssText() {
    std::ifstream f(ToUtf8(ThemePath()), std::ios::binary);
    if (!f) return L"";
    std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return std::wstring(s.begin(), s.end());
}

// ===== Backend sans UI (dossier data, pas de changement visuel) =====
void Log(const std::wstring& msg);
std::wstring GetDataDir() {
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        std::wstring d = std::wstring(path) + L"\\Nav++";
        CreateDirectoryW(d.c_str(), nullptr);
        return d;
    }
    return L".";
}
std::string ToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) return {};
    std::string s(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}
void History_Add(const std::wstring& url, const std::wstring& title) {
    if (url.empty() || url.find(L"about:") == 0) return;
    if (url.find(L"file:///") == 0) return; // pas de ? interne dans historique
    if (url.find(L"file:///profile") == 0) return;
    std::wstring dir = GetDataDir();
    // append + timestamp, pas de doublon consecutif
    std::wofstream f(dir + L"\\history.txt", std::ios::app);
    if (!f) return;
    std::time_t t = std::time(nullptr);
    f << t << L"|" << url << L"|" << title << L"\n";
}
void Favorites_Add(const std::wstring& url, const std::wstring& title) {
    if (url.empty()) return;
    if (url.find(L"file:///") == 0) return; // pas de page interne en fav
    std::wstring dir = GetDataDir();
    std::wofstream f(dir + L"\\favorites.txt", std::ios::app);
    if (f) f << url << L"|" << title << L"\n";
    Log(L"Favori ajoute: " + url);
}
std::vector<std::wstring> Favorites_List() {
    std::vector<std::wstring> out;
    std::wstring dir = GetDataDir();
    std::wofstream create(dir + L"\\favorites.txt", std::ios::app);
    std::wifstream f(dir + L"\\favorites.txt");
    std::wstring line;
    while (std::getline(f, line)) if (!line.empty()) out.push_back(line);
    return out;
}
std::wstring Profile_GetUser() {
    std::wstring dir = GetDataDir();
    std::wifstream f(dir + L"\\profile.txt");
    std::wstring u;
    std::getline(f, u);
    return u;
}
void Profile_SetUser(const std::wstring& user) {
    if (user.empty() || user.size() > 24) return;
    std::wstring dir = GetDataDir();
    std::wofstream f(dir + L"\\profile.txt");
    if (f) f << user;
    // ajoute a la liste des comptes si nouveau
    std::wifstream in(dir + L"\\accounts.txt");
    std::wstring l; bool found = false;
    while (std::getline(in, l)) if (l == user) found = true;
    in.close();
    if (!found) {
        std::wofstream out(dir + L"\\accounts.txt", std::ios::app);
        if (out) out << user << L"\n";
    }
    Log(L"Compte local: " + user);
}
std::vector<std::wstring> Accounts_List() {
    std::vector<std::wstring> out;
    std::wstring dir = GetDataDir();
    std::wofstream c(dir + L"\\accounts.txt", std::ios::app);
    std::wifstream f(dir + L"\\accounts.txt");
    std::wstring l;
    while (std::getline(f, l)) if (!l.empty()) out.push_back(l);
    return out;
}
std::wstring EscapeHtml(const std::wstring& s) {
    std::wstring o;
    for (auto c : s) {
        if (c == L'<') o += L"&lt;"; else if (c == L'>') o += L"&gt;";
        else if (c == L'&') o += L"&amp;"; else if (c == L'"') o += L"&quot;";
        else o += c;
    }
    return o;
}
// vrais onglets internes gaming (pas de .txt externe)
void ShowInternalPage(const std::wstring& kind);
void NavigateTo(const std::wstring& url);

void LogPath(std::wstring &out) {
    out = GetDataDir() + L"\\debug.log";
}
void Log(const std::wstring& msg) {
    std::wstring p = GetDataDir() + L"\\debug.log";
    HANDLE h = CreateFileW(p.c_str(),
        FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
        std::string s(msg.begin(), msg.end());
        s += "\n";
        DWORD w = 0;
        WriteFile(h, s.c_str(), (DWORD)s.size(), &w, nullptr);
        CloseHandle(h);
    }
}

void NavigateTo(const std::wstring& url) {
    if (!webview) {
        Log(L"NavigateTo: webview null, url=" + url);
        return;
    }
    std::wstring finalUrl = url;
    if (finalUrl.find(L"://") == std::wstring::npos)
        finalUrl = L"https://" + finalUrl;
    Log(L"Navigate vers: " + finalUrl);
    webview->Navigate(finalUrl.c_str());
}

void ResizeWebView(HWND hWnd) {
    // resize tous les controleurs (onglets caches gardent les bounds)
    RECT bounds;
    GetClientRect(hWnd, &bounds);
    bounds.left = 62;
    bounds.top = 36 + 52;
    for (auto &t : g_tabs) if (t.ctl) t.ctl->put_Bounds(bounds);
    if (controller) controller->put_Bounds(bounds);
}

int Tab_Find(ICoreWebView2* wv) {
    for (int i = 0; i < (int)g_tabs.size(); i++)
        if (g_tabs[i].wv.Get() == wv) return i;
    return -1;
}
void Tab_SetupEvents(HWND hWnd, ComPtr<ICoreWebView2> wv);
void Tab_Create(HWND hWnd, const std::wstring& url) {
    if (!g_env) return;
    if ((int)g_tabs.size() >= 8) return;
    g_env->CreateCoreWebView2Controller(hWnd,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [hWnd, url](HRESULT r, ICoreWebView2Controller* c) -> HRESULT {
                if (FAILED(r) || !c) return S_OK;
                Tab t;
                t.ctl = c;
                c->put_IsVisible(FALSE);
                c->get_CoreWebView2(&t.wv);
                // masque file interne du log ici, events attaches
                g_tabs.push_back(t);
                int idx = (int)g_tabs.size() - 1;
                g_tabs[idx].title = T(L"new");
                Tab_SetupEvents(hWnd, t.wv);
                // bounds
                RECT b; GetClientRect(hWnd, &b);
                b.left = 62; b.top = 88;
                c->put_Bounds(b);
                Tab_Switch(hWnd, idx);
                if (!url.empty()) {
                    std::wstring f = url;
                    if (f.find(L"://") == std::wstring::npos) f = L"https://" + f;
                    t.wv->Navigate(f.c_str());
                }
                Tab_Refresh(hWnd);
                return S_OK;
            }).Get());
}
void Tab_Switch(HWND hWnd, int i) {
    if (i < 0 || i >= (int)g_tabs.size()) return;
    for (auto &t : g_tabs) if (t.ctl) t.ctl->put_IsVisible(FALSE);
    g_active = i;
    controller = g_tabs[i].ctl;
    webview = g_tabs[i].wv;
    if (controller) {
        controller->put_IsVisible(TRUE);
        ResizeWebView(hWnd);
        controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }
    // url bar + titre
    if (webview) {
        LPWSTR u = nullptr;
        if (SUCCEEDED(webview->get_Source(&u)) && u) {
            std::wstring url = u;
            if (url.find(L"newtab.html") != std::wstring::npos) SetWindowTextW(hUrlBar, L"");
            else if (url.find(L".html") == std::wstring::npos || url.find(L"http") == 0) SetWindowTextW(hUrlBar, url.c_str());
            CoTaskMemFree(u);
        }
    }
    Tab_Refresh(hWnd);
}
void Tab_Close(HWND hWnd, int i) {
    if (g_tabs.size() <= 1) { DestroyWindow(hWnd); return; }
    if (i < 0 || i >= (int)g_tabs.size()) return;
    if (g_tabs[i].ctl) { g_tabs[i].ctl->put_IsVisible(FALSE); g_tabs[i].ctl->Close(); }
    g_tabs.erase(g_tabs.begin() + i);
    if (g_active >= (int)g_tabs.size()) g_active = (int)g_tabs.size() - 1;
    Tab_Switch(hWnd, g_active < 0 ? 0 : g_active);
    Tab_Refresh(hWnd);
}
void Tab_Refresh(HWND hWnd) {
    RECT rc; GetClientRect(hWnd, &rc);
    int maxX = rc.right - 140; // reserve boutons Windows - carre X
    for (int i = 0; i < 8; i++) {
        bool show = i < (int)g_tabs.size();
        if (hTabBtns[i]) ShowWindow(hTabBtns[i], show ? SW_SHOW : SW_HIDE);
        if (hTabX[i]) ShowWindow(hTabX[i], show ? SW_SHOW : SW_HIDE);
        if (show) {
            int x = 10 + i * 170;
            if (x + 170 > maxX) { ShowWindow(hTabBtns[i], SW_HIDE); ShowWindow(hTabX[i], SW_HIDE); continue; }
            else { ShowWindow(hTabBtns[i], SW_SHOW); ShowWindow(hTabX[i], SW_SHOW); }
            MoveWindow(hTabBtns[i], x, 4, 145, 28, TRUE);
            MoveWindow(hTabX[i], x + 145, 4, 25, 28, TRUE);
            std::wstring t = g_tabs[i].title;
            if (t.size() > 16) t = t.substr(0, 16);
            SetWindowTextW(hTabBtns[i], (L" " + t).c_str());
        }
    }
    if (g_tabPlus) {
        int x = 10 + (int)g_tabs.size() * 170;
        if (x + 30 > maxX) x = maxX - 30;
        MoveWindow(g_tabPlus, x, 4, 30, 28, TRUE);
    }
    InvalidateRect(hWnd, nullptr, FALSE);
}
void Tab_SetupEvents(HWND hWnd, ComPtr<ICoreWebView2> wv);

std::wstring GetNewTabUrl() {
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring d = exe;
    auto p = d.find_last_of(L"\\/");
    if (p != std::wstring::npos) d = d.substr(0, p + 1);
    std::wstring f = d + L"newtab.html";
    if (GetFileAttributesW(f.c_str()) == INVALID_FILE_ATTRIBUTES)
        f = L"C:\\Users\\Motata\\Downloads\\navigateur-vs\\navigateur\\newtab.html"; // dev fallback
    for (auto &c : f) if (c == L'\\') c = L'/';
    std::wstring lang = GetAppLang();
    std::wstring th = ThemePath();
    for (auto &c : th) if (c == L'\\') c = L'/';
    return L"file:///" + f + L"?lang=" + lang + L"&theme=file:///" + th;
}

void OpenDataFile(const std::wstring& name) {
    std::wstring p = GetDataDir() + L"\\" + name;
    ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOW);
}
void WriteUtf8File(const std::wstring& path, const std::wstring& content) {
    int n = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) return;
    std::string s(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, &s[0], n, nullptr, nullptr);
    HANDLE hf = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hf == INVALID_HANDLE_VALUE) return;
    DWORD w = 0;
    BYTE bom[3] = {0xEF, 0xBB, 0xBF};
    WriteFile(hf, bom, 3, &w, nullptr);
    WriteFile(hf, s.c_str(), (DWORD)s.size(), &w, nullptr);
    CloseHandle(hf);
}
void ShowInternalPage(const std::wstring& kind) {
    std::wstring dir = GetDataDir();
    std::wstring file = dir + L"\\" + kind + L".html";
    std::wstring html;
    html += L"<!DOCTYPE html><html><head><meta charset='utf-8'><style>"
      L"*{font-family:'Segoe UI',sans-serif}body{background:#202124;color:#e8eaed;margin:0;padding:32px}"
      L"h1{color:#e8eaed;font-size:22px;font-weight:400}"
      L"a{color:#e8eaed;text-decoration:none;display:block;background:#303134;border-radius:8px;padding:12px 16px;margin:8px 0}"
      L"a:hover{background:#3c4043}"
      L".bar{display:flex;gap:10px;margin:18px 0}button{background:#303134;color:#e8eaed;border:1px solid #5f6368;border-radius:16px;padding:8px 16px;cursor:pointer}button:hover{background:#3c4043}"
      L"</style>";
    // custom.css utilisateur injecte (themes)
    html += L"<style>" + ThemeCssText() + L"</style></head><body>";
    if (kind == L"history") {
        html += L"<h1>" + T(L"hist") + L"</h1><div class='bar'><button onclick='history.back()'>Retour</button></div>";
        std::wifstream f(dir + L"\\history.txt");
        std::vector<std::wstring> lines; std::wstring l;
        while (std::getline(f, l)) if (!l.empty()) lines.push_back(l);
        for (int i = (int)lines.size() - 1; i >= 0 && i > (int)lines.size() - 100; i--) {
            auto pos = lines[i].find(L'|');
            std::wstring url = pos != std::wstring::npos ? lines[i].substr(pos + 1) : lines[i];
            auto p2 = url.find(L'|'); if (p2 != std::wstring::npos) url = url.substr(0, p2);
            html += L"<a href='" + EscapeHtml(url) + L"'>" + EscapeHtml(url) + L"</a>";
        }
    } else if (kind == L"favorites") {
        html += L"<h1>" + T(L"fav") + L"</h1><div class='bar'><button onclick='history.back()'>Retour</button></div>";
        for (auto &e : Favorites_List()) {
            auto p = e.find(L'|'); std::wstring url = p != std::wstring::npos ? e.substr(0, p) : e;
            html += L"<a href='" + EscapeHtml(url) + L"'>" + EscapeHtml(url) + L"</a>";
        }
    } else if (kind == L"downloads") {
        html += L"<h1>" + T(L"dl") + L"</h1>"
          L"<div class='bar'><button onclick='history.back()'>Retour</button></div>";
    } else if (kind == L"profile") {
        std::wstring cur = Profile_GetUser();
        if (cur.empty()) cur = L"Invite";
        html += L"<h1>" + T(L"acc") + L" : " + EscapeHtml(cur) + L"</h1>"
          L"<div class='bar'><button onclick='history.back()'>Retour</button></div>";
        for (auto &a : Accounts_List()) {
            html += L"<a href='file:///profile?action=switch&user=" + EscapeHtml(a) + L"'>" + EscapeHtml(a);
            if (a == Profile_GetUser()) html += L" (actuel)";
            html += L"</a>";
        }
        html += L"<div class='bar' style='margin-top:16px'><input id='nu' placeholder='Nouveau pseudo...' style='background:#303134;color:#fff;border:1px solid #5f6368;border-radius:8px;padding:8px 12px'>"
          L"<button onclick=\"location.href='file:///profile?action=create&user='+encodeURIComponent(document.getElementById('nu').value)\">+ Creer</button></div>";
    } else if (kind == L"settings") {
        html += L"<h1>Parametres - Themes</h1><div class='bar'><button onclick='history.back()'>Retour</button>"
          L"<button onclick=\"location.href='file:///settings?action=reload'\">Recharger</button>"
          L"<button onclick=\"location.href='file:///settings?action=open'\">Ouvrir custom.css</button></div>";
        html += L"<div class='bar'>"
          L"<button onclick=\"location.href='file:///settings?action=preset&name=chrome'\">Chrome</button>"
          L"<button onclick=\"location.href='file:///settings?action=preset&name=gaming'\">Gaming</button>"
          L"<button onclick=\"location.href='file:///settings?action=preset&name=light'\">Clair</button>"
          L"<button onclick=\"location.href='file:///settings?action=preset&name=blue'\">Bleu</button></div>";
        // editeur : contenu actuel
        std::ifstream cf(ToUtf8(ThemePath()), std::ios::binary);
        std::string cur((std::istreambuf_iterator<char>(cf)), std::istreambuf_iterator<char>());
        std::wstring wcur(cur.begin(), cur.end());
        // echappe </textarea>
        html += L"<textarea id='css' style='width:100%;height:220px;background:#303134;color:#e8eaed;border:1px solid #5f6368;border-radius:8px;padding:12px;font-family:Consolas,monospace'>" + EscapeHtml(wcur) + L"</textarea>";
        html += L"<div class='bar' style='margin-top:10px'><button onclick=\"location.href='file:///settings?action=save&css='+encodeURIComponent(document.getElementById('css').value)\">Appliquer</button></div>"
          L"<p style='color:#9aa0a6'>Change les --bg --toolbar --text --accent --hover puis Appliquer. Clique un preset pour remplir.</p>";
    }
    html += L"</body></html>";
    WriteUtf8File(file, html);
    std::wstring url = file;
    for (auto &c : url) if (c == L'\\') c = L'/';
    NavigateTo(L"file:///" + url);
    Log(L"Onglet interne: " + kind);
}

// ===== Boutons gaming style maquette : vraies icones Segoe MDL2 + hover anim =====
static int g_hoverId = 0;
HWND GamingBtn(HWND parent, const wchar_t* txt, int x, int y, int w, int h, int id) {
    HWND b = CreateWindowW(L"BUTTON", txt, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, nullptr, nullptr);
    TrackMouseEvent(nullptr); // warmup
    return b;
}
const wchar_t* MdlIcon(int id) {
    if (id >= 100 && id < 108) return L"";
    if (id >= 200 && id < 208) return L"\uE8BB"; // x close onglet
    switch (id) {
    case 1: return L"\uE72B";  // back
    case 5: return L"\uE72A";  // forward
    case 2: return L"\uE72C";  // refresh
    case 6: return L"\uE734";  // star
    case 7: return L"\uE77B";  // contact/profil
    case 8: return L"\uE712";  // more ...
    case 4: return L"\uE71A";  // go fleche (pas de +)
    case 10: return L"\uE710"; // + tab
    case 11: return L"\uE8BB"; // close tab x icone
    case 20: return L"\uE10F"; // home / newtab (100% MDL2, fini emoji ?)
    case 21: return L"\uE81C"; // history
    case 22: return L"\uE734"; // fav star
    case 23: return L"\uE896"; // download
    case 24: return L"\uE713"; // settings
    default: return L"\uE710";
    }
}
void DrawGamingBtn(LPDRAWITEMSTRUCT d) {
    bool sidebar = (d->CtlID >= 20 && d->CtlID <= 24);
    bool istop = (d->CtlID >= 30 && d->CtlID <= 32);
    bool istab = (d->CtlID >= 100 && d->CtlID < 108);
    bool isX = (d->CtlID >= 200 && d->CtlID < 208);
    bool active = (d->CtlID == 20);
    if (istab) active = (g_active >= 0 && d->CtlID == 100 + g_active);
    bool pressed = (d->itemState & ODS_SELECTED);
    bool hover = (g_hoverId == (int)d->CtlID);
    // style theme custom.css (defaut Chrome gris)
    COLORREF parent = sidebar ? g_theme.bg : istop ? g_theme.bg : istab ? (active ? RGB(53, 54, 58) : g_theme.bg) : g_theme.bg;
    // onglet actif = toolbar un peu plus clair, lisible sur tous themes
    if (istab && active) parent = g_theme.toolbar == g_theme.bg ? RGB(53,54,58) : g_theme.toolbar;
    COLORREF bg = parent;
    if (pressed) bg = g_theme.hover;
    else if (hover) bg = g_theme.hover;
    HBRUSH b = CreateSolidBrush(bg);
    FillRect(d->hDC, &d->rcItem, b);
    DeleteObject(b);
    if (d->CtlID >= 30 && d->CtlID <= 32) {
        // boutons Windows style Chrome : - carre X, fond rouge que pour X hover
        if (d->CtlID == 30 && hover) {
            HBRUSH r = CreateSolidBrush(RGB(232, 17, 35));
            FillRect(d->hDC, &d->rcItem, r);
            DeleteObject(r);
        }
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, RGB(220, 221, 225));
        const wchar_t* t = d->CtlID == 30 ? L"\uE8BB" : d->CtlID == 31 ? L"\uE921" : L"\uE922";
        HFONT f = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, L"Segoe MDL2 Assets");
        HFONT o = (HFONT)SelectObject(d->hDC, f);
        DrawTextW(d->hDC, t, -1, &d->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(d->hDC, o);
        DeleteObject(f);
        return;
    }
    // style theme : gris, hover blanc, actif accent
    bool isActiveTab = (istab && active) || (isX && (d->CtlID - 200 == g_active));
    if (isX) {
        // fond = fond de son onglet
        HBRUSH bx = CreateSolidBrush((d->CtlID - 200 == g_active) ? (hover ? RGB(65,66,71) : RGB(53,54,58)) : (hover ? RGB(60,60,65) : RGB(32,33,36)));
        FillRect(d->hDC, &d->rcItem, bx);
        DeleteObject(bx);
    }
    SetBkMode(d->hDC, TRANSPARENT);
    COLORREF tc = hover ? RGB(255, 255, 255) : g_theme.text;
    if (isActiveTab) tc = RGB(255, 255, 255);
    if (sidebar && active && !hover) tc = g_theme.accent;
    SetTextColor(d->hDC, tc);
    // onglets 100+ : vrai titre texte style Chrome (pas d'icone)
    if (d->CtlID >= 100 && d->CtlID < 108) {
        HFONT f = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(d->hDC, f);
        wchar_t txt[64] = {};
        GetWindowTextW(d->hwndItem, txt, 64);
        RECT tr = d->rcItem; tr.left += 8;
        bool isActive = (g_active >= 0 && d->CtlID == 100 + g_active);
        if (isActive) SetTextColor(d->hDC, RGB(255, 255, 255));
        DrawTextW(d->hDC, txt, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(d->hDC, old);
        DeleteObject(f);
        return;
    }
    // 100% MDL2, zero emoji
    const wchar_t* txt = MdlIcon(d->CtlID);
    const wchar_t* font = L"Segoe MDL2 Assets";
    int fs = sidebar ? 20 : 16;
    HFONT f = CreateFontW(fs, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, 0, font);
    HFONT old = (HFONT)SelectObject(d->hDC, f);
    DrawTextW(d->hDC, txt, -1, &d->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(d->hDC, old);
    DeleteObject(f);
}
void Tab_SetupEvents(HWND hWnd, ComPtr<ICoreWebView2> wv) {
    // capture wv brut pour retrouver l'onglet (multi-onglets)
    ICoreWebView2* raw = wv.Get();
    wv->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>(
        [hWnd, raw](ICoreWebView2*, IUnknown*) -> HRESULT {
            int idx = Tab_Find(raw);
            ComPtr<ICoreWebView2> my;
            if (idx >= 0) my = g_tabs[idx].wv;
            else return S_OK;
            LPWSTR u = nullptr;
            if (SUCCEEDED(my->get_Source(&u)) && u) {
                std::wstring url = u;
                std::wstring friendly, title = T(L"new");
                if (url.find(L"newtab.html") != std::wstring::npos) { friendly = L""; title = T(L"new"); }
                else if (url.find(L"history.html") != std::wstring::npos) { friendly = T(L"hist"); title = T(L"hist"); }
                else if (url.find(L"favorites.html") != std::wstring::npos) { friendly = T(L"fav"); title = T(L"fav"); }
                else if (url.find(L"downloads.html") != std::wstring::npos) { friendly = T(L"dl"); title = T(L"dl"); }
                else if (url.find(L"profile.html") != std::wstring::npos || url.find(L"file:///profile") == 0) { friendly = T(L"acc"); title = T(L"acc"); }
                else if (url.find(L"settings.html") != std::wstring::npos || url.find(L"file:///settings") == 0) { friendly = L"Parametres"; title = L"Parametres"; }
                else {
                    friendly = url;
                    auto p = url.find(L"://");
                    std::wstring dom = p != std::wstring::npos ? url.substr(p + 3) : url;
                    auto s = dom.find(L'/'); if (s != std::wstring::npos) dom = dom.substr(0, s);
                    auto q = dom.find(L'?'); if (q != std::wstring::npos) dom = dom.substr(0, q);
                    if (dom.size() > 18) dom = dom.substr(0, 18);
                    title = dom.empty() ? L"Nouvel onglet" : dom;
                }
                g_tabs[idx].title = title;
                if (idx == g_active) {
                    SetWindowTextW(hUrlBar, friendly.c_str());
                    Tab_Refresh(hWnd);
                } else Tab_Refresh(hWnd);
                LPWSTR t = nullptr;
                std::wstring dt;
                if (SUCCEEDED(my->get_DocumentTitle(&t)) && t) { dt = t; CoTaskMemFree(t); }
                History_Add(url, dt);
                CoTaskMemFree(u);
            }
            return S_OK;
        }).Get(), nullptr);
    wv->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
        [](ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs* a) -> HRESULT {
            BOOL ok = FALSE; if (a) a->get_IsSuccess(&ok);
            Log(ok ? L"Navigation OK" : L"Navigation ECHEC");
            return S_OK;
        }).Get(), nullptr);
    wv->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
        [hWnd, raw](ICoreWebView2*, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
            auto urldec = [](const std::wstring& s) {
                std::wstring d;
                for (size_t i = 0; i < s.size(); i++) {
                    if (s[i] == L'%' && i + 2 < s.size()) { int v = 0; swscanf_s(s.substr(i + 1, 2).c_str(), L"%x", &v); d += (wchar_t)v; i += 2; }
                    else if (s[i] == L'+') d += L' ';
                    else d += s[i];
                }
                return d;
            };
            LPWSTR uri = nullptr;
            if (SUCCEEDED(args->get_Uri(&uri)) && uri) {
                std::wstring u = uri;
                if (u.find(L"file:///profile?action=") != std::wstring::npos) {
                    auto q = u.find(L"user=");
                    if (q != std::wstring::npos) {
                        std::wstring d = urldec(u.substr(q + 5));
                        if (!d.empty()) Profile_SetUser(d);
                    }
                    args->put_Cancel(TRUE);
                    ShowInternalPage(L"profile");
                } else if (u.find(L"file:///settings?action=") != std::wstring::npos) {
                    args->put_Cancel(TRUE);
                    if (u.find(L"action=save") != std::wstring::npos) {
                        auto q = u.find(L"css=");
                        if (q != std::wstring::npos) {
                            std::wstring css = urldec(u.substr(q + 4));
                            // urldec donne utf16 approx, ecrit tel quel
                            WriteUtf8File(ThemePath(), css);
                        }
                        Theme_Load();
                        InvalidateRect(hWnd, nullptr, TRUE);
                    } else if (u.find(L"action=preset") != std::wstring::npos) {
                        auto q = u.find(L"name=");
                        std::wstring n = q != std::wstring::npos ? urldec(u.substr(q + 5)) : L"chrome";
                        auto amp = n.find(L'&'); if (amp != std::wstring::npos) n = n.substr(0, amp);
                        WriteUtf8File(ThemePath(), ThemePreset(n));
                        Theme_Load();
                        InvalidateRect(hWnd, nullptr, TRUE);
                    } else if (u.find(L"action=open") != std::wstring::npos) {
                        ShellExecuteW(nullptr, L"open", ThemePath().c_str(), nullptr, nullptr, SW_SHOW);
                    } else if (u.find(L"action=reload") != std::wstring::npos) {
                        Theme_Load();
                        InvalidateRect(hWnd, nullptr, TRUE);
                    }
                    ShowInternalPage(L"settings");
                }
                CoTaskMemFree(uri);
            }
            return S_OK;
        }).Get(), nullptr);
    ComPtr<ICoreWebView2_4> wv4;
    if (SUCCEEDED(wv.As(&wv4)) && wv4) {
        wv4->add_DownloadStarting(Callback<ICoreWebView2DownloadStartingEventHandler>(
            [](ICoreWebView2*, ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT {
                if (!args) return S_OK;
                ICoreWebView2DownloadOperation* op = nullptr;
                if (SUCCEEDED(args->get_DownloadOperation(&op)) && op) {
                    wchar_t* up = _wgetenv(L"USERPROFILE");
                    std::wstring folder = up ? std::wstring(up) + L"\\Downloads" : GetDataDir();
                    CreateDirectoryW(folder.c_str(), nullptr);
                    Log(L"Download demarre vers: " + folder);
                    op->add_StateChanged(Callback<ICoreWebView2StateChangedEventHandler>(
                        [](ICoreWebView2DownloadOperation* o, IUnknown*) -> HRESULT {
                            COREWEBVIEW2_DOWNLOAD_STATE st;
                            if (SUCCEEDED(o->get_State(&st))) {
                                if (st == COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED) Log(L"Download termine");
                                if (st == COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED) Log(L"Download interrompu");
                            }
                            return S_OK;
                        }).Get(), nullptr);
                    op->Release();
                }
                return S_OK;
            }).Get(), nullptr);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hBtnGo, hBtnBack, hBtnFwd, hBtnReload, hBtnStar, hBtnProfile, hBtnMenu;
    static HWND hTab, hTabPlus, hTabClose, hSideGame, hSideHist, hSideFav, hSideDl, hSideSet;
    static HWND hClose, hMin, hMax;
    static HBRUSH hDarkBrush = nullptr, hTabBrush = nullptr;
    switch (msg) {
    case WM_NCCALCSIZE:
        if (wParam) return 0;
        break;
    case WM_NCHITTEST: {
        POINT p = {LOWORD(lParam), HIWORD(lParam)};
        ScreenToClient(hWnd, &p);
        RECT rc; GetClientRect(hWnd, &rc);
        // 1) ne jamais voler les clics des boutons/onglets
        HWND child = ChildWindowFromPoint(hWnd, p);
        if (child && child != hWnd) return HTCLIENT;
        // 2) resize bords
        if (p.y >= rc.bottom - 8 && p.x >= rc.right - 8) return HTBOTTOMRIGHT;
        if (p.y >= rc.bottom - 8 && p.x <= 8) return HTBOTTOMLEFT;
        if (p.y <= 8 && p.x >= rc.right - 8) return HTTOPRIGHT;
        if (p.y <= 8 && p.x <= 8) return HTTOPLEFT;
        if (p.y >= rc.bottom - 5) return HTBOTTOM;
        if (p.x <= 5) return HTLEFT;
        if (p.x >= rc.right - 5) return HTRIGHT;
        // 3) drag barre onglets vide (fini le bug deplacage)
        if (p.y < 36) return HTCAPTION;
        break;
    }
    case WM_CREATE: {
        Theme_Load();
        DeleteFileW((GetDataDir() + L"\\debug.log").c_str());
        LPWSTR ver = nullptr;
        HRESULT hrVer = GetAvailableCoreWebView2BrowserVersionString(nullptr, &ver);
        if (SUCCEEDED(hrVer) && ver) {
            Log(std::wstring(L"Runtime version: ") + ver);
            CoTaskMemFree(ver);
        } else {
            wchar_t b[128];
            swprintf_s(b, L"GetVersion FAILED hr=0x%08X", (unsigned)hrVer);
            Log(b);
        }

        hBtnBack = GamingBtn(hWnd, L"", 70, 44, 34, 34, 1);
        hBtnFwd = GamingBtn(hWnd, L"", 108, 44, 34, 34, 5);
        hBtnReload = GamingBtn(hWnd, L"", 146, 44, 34, 34, 2);
        hUrlBar = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            188, 46, 600, 30, hWnd, (HMENU)3, nullptr, nullptr);
        SendMessageW(hUrlBar, EM_SETCUEBANNER, TRUE, (LPARAM)T(L"ph").c_str());
        hBtnStar = GamingBtn(hWnd, L"", 795, 44, 34, 34, 6);
        hBtnProfile = GamingBtn(hWnd, L"", 835, 44, 36, 34, 7);
        hBtnMenu = GamingBtn(hWnd, L"", 875, 44, 30, 34, 8);
        hBtnGo = GamingBtn(hWnd, L"", 910, 44, 40, 34, 4);
        // Windows style Chrome : - carre X a droite, fini pastilles vertes macOS
        hMin = GamingBtn(hWnd, L"", 1100, 0, 45, 32, 31);
        hMax = GamingBtn(hWnd, L"", 1145, 0, 45, 32, 32);
        hClose = GamingBtn(hWnd, L"", 1190, 0, 45, 32, 30);
        // multi-onglets Chrome : 8 max, titre texte, x icone
        for (int i = 0; i < 8; i++) {
            hTabBtns[i] = GamingBtn(hWnd, L"", 70 + i * 170, 4, 145, 28, 100 + i);
            hTabX[i] = GamingBtn(hWnd, L"", 70 + i * 170 + 145, 4, 25, 28, 200 + i);
            ShowWindow(hTabBtns[i], SW_HIDE);
            ShowWindow(hTabX[i], SW_HIDE);
        }
        hTab = CreateWindowW(L"STATIC", L"",
            WS_CHILD | SS_LEFT,
            70, 4, 10, 28, hWnd, nullptr, nullptr, nullptr);
        hTabClose = GamingBtn(hWnd, L"", 0, 0, 0, 0, 11);
        ShowWindow(hTab, SW_HIDE); ShowWindow(hTabClose, SW_HIDE);
        hTabPlus = GamingBtn(hWnd, L"", 70, 4, 30, 28, 10);
        g_tabPlus = hTabPlus;
        // sidebar
        hSideGame = GamingBtn(hWnd, L"", 8, 100, 46, 46, 20);
        hSideHist = GamingBtn(hWnd, L"", 8, 152, 46, 46, 21);
        hSideFav = GamingBtn(hWnd, L"", 8, 204, 46, 46, 22);
        hSideDl = GamingBtn(hWnd, L"", 8, 256, 46, 46, 23);
        hSideSet = GamingBtn(hWnd, L"", 8, 308, 46, 46, 24);
        // police Chrome clean
        HFONT hUi = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        SendMessageW(hUrlBar, WM_SETFONT, (WPARAM)hUi, TRUE);
        if (!hDarkBrush) hDarkBrush = CreateSolidBrush(RGB(32, 33, 36));
        if (!hTabBrush) hTabBrush = CreateSolidBrush(RGB(50, 52, 57));
        hWebViewParent = hWnd;
        g_mainWnd = hWnd;

        // Entree dans la barre d'adresse = Go
        g_oldEdit = (WNDPROC)SetWindowLongPtrW(hUrlBar, GWLP_WNDPROC, (LONG_PTR)EditSubclass);

        // userData inscriptible meme installe en Program Files (fini bug post-install)
        std::wstring ud = GetDataDir() + L"\\WebViewData";
        CreateDirectoryW(ud.c_str(), nullptr);
        static std::wstring udKeep = ud;
        CreateCoreWebView2EnvironmentWithOptions(nullptr, udKeep.c_str(), nullptr,
            Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
                [hWnd](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
                    if (FAILED(result) || !env) {
                        wchar_t b[256];
                        swprintf_s(b, L"CreateEnvironment FAILED hr=0x%08X", (unsigned)result);
                        Log(b);
                        MessageBoxW(hWnd, b, L"WebView2 manquant", MB_ICONERROR);
                        return S_OK;
                    }
                    Log(L"Environment OK, multi-onglets...");
                    g_env = env;
                    Tab_Create(hWnd, GetNewTabUrl());
                    return S_OK;
                }).Get());
        break;
    }
    case WM_SIZE: {
        int W = LOWORD(lParam);
        if (hUrlBar) MoveWindow(hUrlBar, 210, 48, max(200, W - 480), 26, TRUE);
        if (hBtnStar) MoveWindow(hBtnStar, max(200, W - 265), 44, 34, 34, TRUE);
        if (hBtnProfile) MoveWindow(hBtnProfile, max(200, W - 227), 44, 36, 34, TRUE);
        if (hBtnMenu) MoveWindow(hBtnMenu, max(200, W - 187), 44, 30, 34, TRUE);
        if (hBtnGo) MoveWindow(hBtnGo, max(200, W - 153), 44, 40, 34, TRUE);
        if (hMin) MoveWindow(hMin, W - 135, 0, 45, 32, TRUE);
        if (hMax) MoveWindow(hMax, W - 90, 0, 45, 32, TRUE);
        if (hClose) MoveWindow(hClose, W - 45, 0, 45, 32, TRUE);
        ResizeWebView(hWnd);
        Tab_Refresh(hWnd);
        InvalidateRect(hWnd, nullptr, TRUE);
        break;
    }
    case WM_CTLCOLORSTATIC: {
        SetBkColor((HDC)wParam, g_theme.bg);
        SetTextColor((HDC)wParam, RGB(220, 221, 225));
        if (!g_bgBrush) g_bgBrush = CreateSolidBrush(g_theme.bg);
        return (LRESULT)g_bgBrush;
    }
    case WM_CTLCOLOREDIT: {
        SetBkColor((HDC)wParam, g_theme.bg);
        SetTextColor((HDC)wParam, RGB(220, 221, 225));
        if (!g_barBrush) g_barBrush = CreateSolidBrush(g_theme.bg);
        return (LRESULT)g_barBrush;
    }
    case WM_DRAWITEM:
        DrawGamingBtn((LPDRAWITEMSTRUCT)lParam);
        return TRUE;
    case WM_MOUSEMOVE: {
        POINT p = {LOWORD(lParam), HIWORD(lParam)};
        HWND child = ChildWindowFromPoint(hWnd, p);
        int id = child ? GetDlgCtrlID(child) : 0;
        if (id != g_hoverId) {
            g_hoverId = id;
            InvalidateRect(hWnd, nullptr, FALSE);
            HWND ctrls[] = {hBtnBack,hBtnFwd,hBtnReload,hBtnStar,hBtnProfile,hBtnMenu,hBtnGo,hTabPlus,hTabClose,hSideGame,hSideHist,hSideFav,hSideDl,hSideSet,hClose,hMin,hMax,
                hTabBtns[0],hTabBtns[1],hTabBtns[2],hTabBtns[3],hTabBtns[4],hTabBtns[5],hTabBtns[6],hTabBtns[7],
                hTabX[0],hTabX[1],hTabX[2],hTabX[3],hTabX[4],hTabX[5],hTabX[6],hTabX[7]};
            for (auto c : ctrls) if (c) InvalidateRect(c, nullptr, FALSE);
        }
        TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, hWnd, 0};
        TrackMouseEvent(&t);
        break;
    }
    case WM_MOUSELEAVE:
        g_hoverId = 0;
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH topBg = CreateSolidBrush(g_theme.bg);
        RECT top = {0, 0, rc.right, 36};
        FillRect(hdc, &top, topBg);
        RECT bar = {0, 36, rc.right, 88};
        FillRect(hdc, &bar, topBg);
        RECT side = {0, 88, 62, rc.bottom};
        FillRect(hdc, &side, topBg);
        DeleteObject(topBg);
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 4) { // Go / Entree
            wchar_t buf[2048] = {};
            GetWindowTextW(hUrlBar, buf, 2048);
            std::wstring s = buf;
            // si pas d'URL mais recherche -> DuckDuckGo
            if (s.find(L"://") == std::wstring::npos && s.find(L".") == std::wstring::npos
                && s.find(L"file:") != 0 && s.find(L"Rechercher") == std::wstring::npos) {
                s = L"https://duckduckgo.com/?q=" + s;
                NavigateTo(s);
            } else {
                NavigateTo(buf);
            }
        }
        if (LOWORD(wParam) == 1 && webview) webview->GoBack();
        if (LOWORD(wParam) == 5 && webview) webview->GoForward();
        if (LOWORD(wParam) == 2 && webview) webview->Reload();
        if (LOWORD(wParam) == 6) { // star : ajoute
            wchar_t buf[2048] = {};
            GetWindowTextW(hUrlBar, buf, 2048);
            std::wstring s = buf;
            if (s.find(L"http") == 0) Favorites_Add(s, L"");
            ShowInternalPage(L"favorites");
        }
        if (LOWORD(wParam) == 22) {
            ShowInternalPage(L"favorites");
        }
        if (LOWORD(wParam) == 7) {
            ShowInternalPage(L"profile");
        }
        if (LOWORD(wParam) == 8) {
            MessageBoxW(hWnd, L"Menu : Historique / Favoris / Downloads dans %APPDATA%\\Nav++", L"Menu", MB_ICONINFORMATION);
        }
        if (LOWORD(wParam) == 10 || LOWORD(wParam) == 20) { // + / home : nouvel onglet
            Tab_Create(hWnd, GetNewTabUrl());
        }
        if (LOWORD(wParam) == 11) {
            Tab_Close(hWnd, g_active);
        }
        // select onglet 100-107, close 200-207
        if (LOWORD(wParam) >= 100 && LOWORD(wParam) < 108) {
            Tab_Switch(hWnd, LOWORD(wParam) - 100);
        }
        if (LOWORD(wParam) >= 200 && LOWORD(wParam) < 208) {
            Tab_Close(hWnd, LOWORD(wParam) - 200);
        }
        if (LOWORD(wParam) == 21) {
            ShowInternalPage(L"history");
        }
        if (LOWORD(wParam) == 23) {
            ShowInternalPage(L"downloads");
        }
        if (LOWORD(wParam) == 24) {
            ShowInternalPage(L"settings");
        }
        if (LOWORD(wParam) == 30) DestroyWindow(hWnd);
        if (LOWORD(wParam) == 31) ShowWindow(hWnd, SW_MINIMIZE);
        if (LOWORD(wParam) == 32) {
            if (IsZoomed(hWnd)) ShowWindow(hWnd, SW_RESTORE);
            else ShowWindow(hWnd, SW_MAXIMIZE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmd) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = CreateSolidBrush(RGB(32, 33, 36));
    wc.lpszClassName = L"NavPlusPlus";
    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(WS_EX_APPWINDOW, L"NavPlusPlus", L"Nav++",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 800, nullptr, nullptr, hInst, nullptr);
    COLORREF dark = RGB(32, 33, 36);
    DwmSetWindowAttribute(hWnd, 34, &dark, sizeof(dark)); // DWMWA_BORDER_COLOR
    BOOL d = TRUE;
    DwmSetWindowAttribute(hWnd, 20, &d, sizeof(d)); // dark mode
    ShowWindow(hWnd, nCmd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    CoUninitialize();
    return 0;
}
