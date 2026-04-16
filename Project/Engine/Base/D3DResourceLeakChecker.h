#pragma once
// ★ここが重要！Windows.hを最初に読み込むことで "1" が使えるようになる
#include <Windows.h> 

#include <d3d12.h>
#include <dxgidebug.h>
#include <wrl.h>

/// <summary>
/// リソースリークチェッカー
/// </summary>
struct D3DResourceLeakChecker{
    ~D3DResourceLeakChecker(){
        // ★ "1" 付きに戻す
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if(SUCCEEDED(DXGIGetDebugInterface1(0,IID_PPV_ARGS(&debug)))){
            debug->ReportLiveObjects(DXGI_DEBUG_ALL,DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_APP,DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_D3D12,DXGI_DEBUG_RLO_ALL);
        }
    }
};