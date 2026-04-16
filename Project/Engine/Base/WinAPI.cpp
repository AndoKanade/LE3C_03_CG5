#include "WinAPI.h"
#include "imgui_impl_win32.h"

#ifdef USE_IMGUI
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lPalam);
#endif

LRESULT CALLBACK WinAPI::WindowProc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam){
#ifdef USE_IMGUI
	if(ImGui_ImplWin32_WndProcHandler(hwnd,msg,wparam,lparam)){
		return true;
	}
#endif

	switch(msg){
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd,msg,wparam,lparam);
}

void WinAPI::Initialize(){
	HRESULT hr = CoInitializeEx(0,COINIT_MULTITHREADED);

	wc.lpfnWndProc = WindowProc;
	wc.lpszClassName = L"CG2WindowClass";
	wc.hInstance = GetModuleHandle(nullptr);
	wc.hCursor = LoadCursor(nullptr,IDC_ARROW);

	RegisterClass(&wc);

	RECT wrc = {0, 0, kClientWidth, kCliantHeight};

	AdjustWindowRect(&wrc,WS_OVERLAPPEDWINDOW,false);

	hwnd = CreateWindow(
		wc.lpszClassName,
		L"GE3",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	ShowWindow(hwnd,SW_SHOW);
}

void WinAPI::Update(){}

bool WinAPI::ProcessMessage(){
	MSG msg{};

	if(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if(msg.message == WM_QUIT){
		return true;
	}

	return false;
}

void WinAPI::Finalize(){
	CloseWindow(hwnd);
	CoUninitialize();
}