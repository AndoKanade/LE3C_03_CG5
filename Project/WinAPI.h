#pragma once
#include "externals/imgui/imgui/imgui.h"
#include <Windows.h>
#include <cstdint>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lPalam);

class WinAPI{
public:
    static LRESULT CALLBACK WindowProc(HWND hwnd,UINT msg,WPARAM wparam,
        LPARAM lparam);

    void Initialize();
    void Update();
    void Finalize();

    bool ProcessMessage();

    HWND GetHwnd() const{ return hwnd; }
    HINSTANCE GetHinstance() const{ return wc.hInstance; }

    static const int32_t kClientWidth = 1280;
    static const int32_t kCliantHeight = 720;

private:
    HWND hwnd = nullptr;
    WNDCLASS wc{};
};
