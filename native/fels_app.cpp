// The window, the Direct3D 11 device and the frame loop of a Dear ImGui program, as a few C
// functions, so the Fels side does not have to speak COM. Built into cimgui.dll with Dear
// ImGui, cimgui and the Win32 and DirectX 11 backends (see build.sh).
//
// Adapted from Dear ImGui's examples/example_win32_directx11/main.cpp (MIT license,
// Copyright (c) 2014-2026 Omar Cornut).

#include "imgui.h"
#include "imgui_internal.h"
#include "implot.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <windows.h>
#include <dwmapi.h>
#include <commdlg.h>
#include <string.h>

#define FELS_API extern "C" __declspec(dllexport)

// Flags of fels_app_open.
enum {
    FELS_APP_DOCKING   = 1,  // panels can be docked into each other and into the window
    FELS_APP_VIEWPORTS = 2,  // panels can leave the main window and become windows of their own
    FELS_APP_KEYBOARD  = 4,  // the keyboard moves between widgets
};

static HWND                    g_hwnd = nullptr;
static WNDCLASSEXW             g_wc = {};
static ID3D11Device*           g_device = nullptr;
static ID3D11DeviceContext*    g_context = nullptr;
static IDXGISwapChain*         g_swap_chain = nullptr;
static ID3D11RenderTargetView* g_target = nullptr;
static bool                    g_occluded = false;
static UINT                    g_resize_width = 0, g_resize_height = 0;
static float                   g_dpi_scale = 1.0f;
static char                    g_ini_path[1024] = {};

static void create_target() {
    ID3D11Texture2D* back = nullptr;
    g_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back));
    if (back) {
        g_device->CreateRenderTargetView(back, nullptr, &g_target);
        back->Release();
    }
}

static void cleanup_target() {
    if (g_target) { g_target->Release(); g_target = nullptr; }
}

static bool create_device(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL level;
    const D3D_FEATURE_LEVEL levels[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
        D3D11_SDK_VERSION, &sd, &g_swap_chain, &g_device, &level, &g_context);
    if (res == DXGI_ERROR_UNSUPPORTED)  // no GPU: the WARP software driver
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
            D3D11_SDK_VERSION, &sd, &g_swap_chain, &g_device, &level, &g_context);
    if (res != S_OK) return false;
    IDXGIFactory* factory = nullptr;
    if (SUCCEEDED(g_swap_chain->GetParent(IID_PPV_ARGS(&factory)))) {
        factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        factory->Release();
    }
    create_target();
    return true;
}

static void cleanup_device() {
    cleanup_target();
    if (g_swap_chain) { g_swap_chain->Release(); g_swap_chain = nullptr; }
    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static LRESULT WINAPI window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (wparam == SIZE_MINIMIZED) return 0;
        g_resize_width = (UINT)LOWORD(lparam);
        g_resize_height = (UINT)HIWORD(lparam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wparam & 0xfff0) == SC_KEYMENU) return 0;  // no ALT menu
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
}

// Open the main window, `title` in UTF-8, `width` x `height` at 96 dpi (scaled to the
// monitor), with Dear ImGui drawn by Direct3D 11. `ini_path` is where the layout of the
// panels is kept between runs; NULL keeps none. 1 when it worked.
FELS_API int fels_app_open(const char* title, int width, int height, int flags, const char* ini_path) {
    ImGui_ImplWin32_EnableDpiAwareness();
    g_dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    wchar_t wide_title[256];
    if (::MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_title, 256) == 0) wide_title[0] = 0;
    g_wc = { sizeof(g_wc), CS_CLASSDC, window_proc, 0L, 0L, ::GetModuleHandleW(nullptr), nullptr,
             ::LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW), nullptr, nullptr, L"FelsImGuiApp", nullptr };
    ::RegisterClassExW(&g_wc);
    g_hwnd = ::CreateWindowW(g_wc.lpszClassName, wide_title, WS_OVERLAPPEDWINDOW, 100, 100,
        (int)(width * g_dpi_scale), (int)(height * g_dpi_scale), nullptr, nullptr, g_wc.hInstance, nullptr);
    if (!g_hwnd || !create_device(g_hwnd)) {
        cleanup_device();
        if (g_hwnd) ::DestroyWindow(g_hwnd);
        ::UnregisterClassW(g_wc.lpszClassName, g_wc.hInstance);
        return 0;
    }
    ::ShowWindow(g_hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    if (flags & FELS_APP_KEYBOARD) io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (flags & FELS_APP_DOCKING) io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (flags & FELS_APP_VIEWPORTS) io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    if (ini_path) {
        strncpy(g_ini_path, ini_path, sizeof(g_ini_path) - 1);
        io.IniFilename = g_ini_path;
    } else {
        io.IniFilename = nullptr;
    }

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(g_dpi_scale);
    style.FontScaleDpi = g_dpi_scale;
    io.ConfigDpiScaleFonts = true;
    io.ConfigDpiScaleViewports = true;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_device, g_context);
    return 1;
}

// Handle the window's messages and begin a frame. 0 when the window was closed: then call
// fels_app_close and draw nothing more.
FELS_API int fels_app_begin_frame(void) {
    for (;;) {
        MSG msg;
        while (::PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) return 0;
        }
        // A window that cannot be seen is not drawn.
        if (g_occluded && g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            ::Sleep(10);
            continue;
        }
        g_occluded = false;
        break;
    }
    if (g_resize_width != 0 && g_resize_height != 0) {
        cleanup_target();
        g_swap_chain->ResizeBuffers(0, g_resize_width, g_resize_height, DXGI_FORMAT_UNKNOWN, 0);
        g_resize_width = g_resize_height = 0;
        create_target();
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    return 1;
}

// Draw the frame on the background `r`, `g`, `b` (0 to 1) and show it.
FELS_API void fels_app_end_frame(float r, float g, float b) {
    ImGui::Render();
    const float clear[4] = { r, g, b, 1.0f };
    g_context->OMSetRenderTargets(1, &g_target, nullptr);
    g_context->ClearRenderTargetView(g_target, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
    HRESULT hr = g_swap_chain->Present(1, 0);  // with vsync
    g_occluded = (hr == DXGI_STATUS_OCCLUDED);
}

// Close the window and free what fels_app_open made.
FELS_API void fels_app_close(void) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    cleanup_device();
    if (g_hwnd) ::DestroyWindow(g_hwnd);
    ::UnregisterClassW(g_wc.lpszClassName, g_wc.hInstance);
    g_hwnd = nullptr;
}

// The scale of the main monitor: 1 at 96 dpi, 1.5 at 144.
FELS_API float fels_app_dpi_scale(void) { return g_dpi_scale; }

// Ask the window to close, as its close button does.
FELS_API void fels_app_quit(void) { if (g_hwnd) ::PostMessageW(g_hwnd, WM_CLOSE, 0, 0); }

// Load the TrueType font at `path` with `size` pixels (at 96 dpi) as the default font. 1 when
// it loaded. Call it after fels_app_open and before the first frame.
FELS_API int fels_app_load_font(const char* path, float size) {
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(path, size);
    if (!font) return 0;
    io.FontDefault = font;
    ImGui::GetStyle().FontSizeBase = size;
    return 1;
}

// Set the style color `idx` (ImGuiCol_*) to `r`, `g`, `b`, `a` (0 to 1).
FELS_API void fels_app_set_color(int idx, float r, float g, float b, float a) {
    if (idx < 0 || idx >= ImGuiCol_COUNT) return;
    ImGui::GetStyle().Colors[idx] = ImVec4(r, g, b, a);
}

// The rounding of windows, frames (buttons, fields), tabs and scrollbars, in pixels at 96 dpi.
FELS_API void fels_app_set_rounding(float window, float frame, float tab, float scrollbar) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = window * g_dpi_scale;
    s.ChildRounding = window * g_dpi_scale;
    s.PopupRounding = window * g_dpi_scale;
    s.FrameRounding = frame * g_dpi_scale;
    s.GrabRounding = frame * g_dpi_scale;
    s.TabRounding = tab * g_dpi_scale;
    s.ScrollbarRounding = scrollbar * g_dpi_scale;
}

// How dense the UI is: the padding inside frames and table cells and the spacing between
// items, in pixels at 96 dpi.
FELS_API void fels_app_set_spacing(float frame_x, float frame_y, float item_x, float item_y, float cell_x, float cell_y) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.FramePadding = ImVec2(frame_x * g_dpi_scale, frame_y * g_dpi_scale);
    s.ItemSpacing = ImVec2(item_x * g_dpi_scale, item_y * g_dpi_scale);
    s.CellPadding = ImVec2(cell_x * g_dpi_scale, cell_y * g_dpi_scale);
}

// The width of the borders of windows, frames and tabs: 0 for none, 1 for a thin line.
FELS_API void fels_app_set_borders(float window, float frame, float tab) {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowBorderSize = window;
    s.ChildBorderSize = window;
    s.PopupBorderSize = window;
    s.FrameBorderSize = frame;
    s.TabBorderSize = tab;
}

// The title bar of the window: dark (1) or light (0) buttons and text, and the color of the bar.
// Windows 10 knows only the first; there the color is left as it is.
FELS_API void fels_app_set_title_bar(int dark, float r, float g, float b) {
    if (!g_hwnd) return;
    BOOL value = dark ? TRUE : FALSE;
    ::DwmSetWindowAttribute(g_hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &value, sizeof(value));
    COLORREF color = RGB((int)(r * 255.0f + 0.5f), (int)(g * 255.0f + 0.5f), (int)(b * 255.0f + 0.5f));
    ::DwmSetWindowAttribute(g_hwnd, 35 /* DWMWA_CAPTION_COLOR */, &color, sizeof(color));
}

// Whether the layout of the panels changed since the last call, when the program keeps it
// itself (no ini file): then it saves igSaveIniSettingsToMemory.
FELS_API int fels_app_layout_changed(void) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantSaveIniSettings) return 0;
    io.WantSaveIniSettings = false;
    return 1;
}

// Scale every text: 1 for the size the font was loaded with.
FELS_API void fels_app_set_font_scale(float scale) { ImGui::GetStyle().FontScaleMain = scale; }

// The file dialog of Windows, to open a file (`save` 0) or to save one (1). `title` and
// `filter` are UTF-8; the filter has its parts between `|`: "CSV files|*.csv|All files|*.*".
// The path chosen goes into `out` (UTF-8, with a NUL); its length, or 0 when the dialog was
// closed without one (or the path does not fit).
FELS_API int fels_app_file_dialog(int save, const char* title, const char* filter, char* out, int out_len) {
    wchar_t wide_title[256];
    if (::MultiByteToWideChar(CP_UTF8, 0, title, -1, wide_title, 256) == 0) wide_title[0] = 0;
    wchar_t wide_filter[512];
    int n = ::MultiByteToWideChar(CP_UTF8, 0, filter, -1, wide_filter, 510);
    if (n <= 0) { wide_filter[0] = 0; n = 1; }
    for (int i = 0; i < n; i++) if (wide_filter[i] == L'|') wide_filter[i] = 0;
    wide_filter[n] = 0;  // the list ends in two NULs
    wchar_t path[1024] = {};
    if (out_len > 0 && out[0]) ::MultiByteToWideChar(CP_UTF8, 0, out, -1, path, 1024);
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFilter = wide_filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = 1024;
    ofn.lpstrTitle = wide_title;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST));
    BOOL ok = save ? ::GetSaveFileNameW(&ofn) : ::GetOpenFileNameW(&ofn);
    if (!ok) return 0;
    int len = ::WideCharToMultiByte(CP_UTF8, 0, path, -1, out, out_len, nullptr, nullptr);
    if (len <= 0) return 0;
    return len - 1;
}

// Columns whose header text goes to the right of its cell, over numbers: bit n for column n of
// the table being declared.
static unsigned long long g_right_headers = 0;

// Declare a column of the current table, its header at the right of the cell when `right`.
FELS_API void fels_app_table_column(const char* label, int flags, float width, int right) {
    ImGuiTable* table = ImGui::GetCurrentTable();
    int n = table ? table->DeclColumnsCount : 0;
    if (n == 0) g_right_headers = 0;
    if (right && n < 64) g_right_headers |= 1ull << n;
    ImGui::TableSetupColumn(label, flags, width, 0);
}

// The row of headers, as TableHeadersRow draws it, with the headers of the columns declared
// `right` at the right of their cells (left of the sort arrow), where the numbers below end.
FELS_API void fels_app_table_headers(void) {
    ImGuiContext& g = *GImGui;
    ImGuiTable* table = g.CurrentTable;
    if (!table) return;
    if (!table->IsLayoutLocked) ImGui::TableUpdateLayout(table);
    ImGui::TableNextRow(ImGuiTableRowFlags_Headers, ImGui::TableGetHeaderRowHeight());
    if (table->HostSkipItems) return;
    const int count = ImGui::TableGetColumnCount();
    for (int n = 0; n < count; n++) {
        if (!ImGui::TableSetColumnIndex(n) && table->LastHeldHeaderColumn != n) continue;
        const char* name = (ImGui::TableGetColumnFlags(n) & ImGuiTableColumnFlags_NoHeaderLabel) ? "" : ImGui::TableGetColumnName(n);
        if (n < 64 && ((g_right_headers >> n) & 1ull)) {
            float w = ImGui::CalcTextSize(name, nullptr, true).x;
            float arrow = 0.0f;
            if ((table->Flags & ImGuiTableFlags_Sortable) && table->Columns[n].SortOrder != -1)
                arrow = ImTrunc(g.FontSize * 0.65f + g.Style.FramePadding.x);
            float room = ImGui::GetContentRegionAvail().x - arrow;
            if (room > w) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + room - w);
        }
        ImGui::PushID(n);
        ImGui::TableHeader(name);
        ImGui::PopID();
    }
}
